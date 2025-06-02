#include "Renderer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <ranges>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "GLFW/glfw3.h"
#include "mle/window/Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace mle::renderer {
namespace {
struct FrameData {
    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence render_finished_fence;
    vk::CommandPool command_pool;
    vk::CommandBuffer cmd;
    vk::QueryPool query_pool;

    // std::vector<BufferHnd> buffers;
    // std::vector<ImageHnd> images;

    std::vector<std::function<void(void)>> delete_queue;
};

struct PhysicalDeviceInfo {
    vk::PhysicalDevice device;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    std::vector<vk::QueueFamilyProperties> queue_family_properties;
};

class Impl {
  public:
    Result init();
    void shutdown();

    void waitIdle();

  private:
    Result createVkInstance();
    Result createVkDebugMessenger();
    Result createVkSurface();
    Result createVkDevice();
    Result pickPhysicalDevice();
    [[nodiscard]] Expected<vk::Format> pickFormat(const std::vector<vk::Format>& candidates, vk::FormatFeatureFlags flags) const;
    Result pickAioQueueIdx();
    Result createVMA();

    [[maybe_unused]] Result createSwapchain();

    void shutdownVkContext();

    void logDevice();

  private:
    vk::Instance vk_instance_;
    vk::DebugUtilsMessengerEXT debug_messenger_;
    vk::SurfaceKHR surface_;
    PhysicalDeviceInfo p_device_;
    vk::Device device_;
    VmaAllocator vma_ = nullptr;
    usize aio_queue_idx_ = max<usize>();
    vk::Queue aio_queue_;
    vk::SwapchainKHR swapchain_;
    vk::Extent2D swapchain_extent_;
    std::vector<vk::Image> swapchain_images_;
    vk::Format depth_format_ = vk::Format::eUndefined;
    vk::Format color_format_ = vk::Format::eUndefined;
    vk::Format swapchain_format_ = vk::Format::eUndefined;

    std::vector<std::function<void(void)>> shutdown_delete_queue_;

    ED ed_;

    // std::vector<ImageHnd> swapchain_images_;

    // usize frame_number_ = 0;
    // u8 current_frame_idx_ = 0;
    // u8 current_swapchain_image_idx_ = max<u8>();
    // std::array<FrameData, 2> frames_;
    //
    // bool swapchain_dirty_ = true;
    // bool frame_active_ = false;
    //
    // vk::CommandPool transfer_ots_command_pool_;
    // vk::CommandBuffer transfer_ots_cmd_;
    // vk::Fence transfer_ots_fence_;
    //
    //
    // ImageHnd default_texture_;

    // bool at_render_pass_ = false;
    // Recti current_render_area_{};
    // PipelineRef current_pipeline_{};
};
std::unique_ptr<Impl> i_;  // NOLINT

VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types,
                                  VkDebugUtilsMessengerCallbackDataEXT const* callback_data, [[maybe_unused]] void* _) {
    std::stringstream formatted_msg;

    formatted_msg << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) << ": ";
    formatted_msg << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_types)) << ":\n";
    formatted_msg << "messageIDName   = <" << callback_data->pMessageIdName << ">\n";
    formatted_msg << "messageIdNumber = <" << callback_data->messageIdNumber << ">\n";
    formatted_msg << "message         = <" << callback_data->pMessage << ">\n";

    if (callback_data->queueLabelCount > 0) {
        formatted_msg << "Queue Labels:\n";
        for (uint32_t i = 0; i < callback_data->queueLabelCount; i++) {
            formatted_msg << "  labelName = <" << callback_data->pQueueLabels[i].pLabelName << ">\n";  // NOLINT
        }
    }
    if (callback_data->cmdBufLabelCount > 0) {
        formatted_msg << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < callback_data->cmdBufLabelCount; i++) {
            formatted_msg << "  labelName = <" << callback_data->pCmdBufLabels[i].pLabelName << ">\n";  // NOLINT
        }
    }
    if (callback_data->objectCount > 0) {
        formatted_msg << "Objects:\n";
        for (uint32_t i = 0; i < callback_data->objectCount; i++) {
            formatted_msg << "  Object " << i;
            if (callback_data->pObjects[i].pObjectName)                                 // NOLINT
                formatted_msg << "<" << callback_data->pObjects[i].pObjectName << ">";  // NOLINT
            formatted_msg << "\n";
            formatted_msg << "    objectType = " << vk::to_string(static_cast<vk::ObjectType>(callback_data->pObjects[i].objectType))  // NOLINT
                          << "\n";
            formatted_msg << "    objectHandle = " << callback_data->pObjects[i].objectHandle << "\n";  // NOLINT
        }
    }

    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            MLE_E("{}", formatted_msg.str());
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            MLE_W("{}", formatted_msg.str());
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            MLE_I("{}", formatted_msg.str());
            break;
        default:
            MLE_T("{}", formatted_msg.str());
            break;
    }

    return VK_FALSE;
}

bool areValidationLayersPresent() {
    auto available_layers_r = vk::enumerateInstanceLayerProperties();
    if (available_layers_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to enumerate Vulkan instance layers! Error code: {}", available_layers_r.result);
        return false;
    }

    MLE_T("Available layers:");
    for (const auto& layer_properties : available_layers_r.value) {
        MLE_T("  {}", layer_properties.layerName.data());
    }

    std::array required_layers = {"VK_LAYER_KHRONOS_validation"};

    for (const char* layer_name : required_layers) {
        bool layer_found = false;
        for (const auto& layer_properties : available_layers_r.value) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }
    return true;
}

Result Impl::init() {
    MLE_I("Initializing the Renderer");

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (isError(createVkInstance())) {
        MLE_E("Failed to create Vulkan instance!");
        shutdown();
        MLE_C("Renderer initialization failed!");
        return Result::INIT_FAILED;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance_);

    if constexpr (IS_DEBUG_BUILD) {
        if (isError(createVkDebugMessenger())) {
            MLE_E("Failed to create Vulkan debug messenger!");
        }
    }

    if (isError(createVkSurface())) {
        MLE_E("Failed to create Vulkan surface!");
        shutdown();
        MLE_C("Renderer initialization failed!");
        return Result::INIT_FAILED;
    }

    if (isError(createVkDevice())) {
        MLE_E("Failed to create Vulkan device!");
        shutdown();
        MLE_C("Renderer initialization failed!");
        return Result::INIT_FAILED;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);

    if (isError(createVMA())) {
        MLE_E("Failed to create Vulkan Memory Allocator!");
        shutdown();
        MLE_C("Renderer initialization failed!");
        return Result::INIT_FAILED;
    }

    MLE_I("Renderer initialized successfully!");
    return Result::OK;
}

Result Impl::createVkInstance() {
    MLE_D("Creating Vulkan instance");

    auto app_info = vk::ApplicationInfo{};
    app_info.pApplicationName = "Unnamed MLE application!";
    app_info.pEngineName = ENGINE_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> instance_extensions;

    u32 surface_extension_count = 0;
    auto* surface_extensions = glfwGetRequiredInstanceExtensions(&surface_extension_count);
    for (u32 i = 0; i < surface_extension_count; ++i) {
        instance_extensions.push_back(surface_extensions[i]);  // NOLINT
    }

    std::vector<const char*> instance_layers;

    auto instance_ci = vk::InstanceCreateInfo{};
    auto instance_debug_messenger_ci = vk::DebugUtilsMessengerCreateInfoEXT{};

    if constexpr (IS_DEBUG_BUILD) {
        if (areValidationLayersPresent()) {
            instance_extensions.push_back(vk::EXTDebugUtilsExtensionName);
            instance_layers.push_back("VK_LAYER_KHRONOS_validation");

            instance_debug_messenger_ci.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            instance_debug_messenger_ci.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
            instance_debug_messenger_ci.pfnUserCallback = debugCallback;
            instance_ci.pNext = &instance_debug_messenger_ci;
        } else {
            MLE_E("Validation layers are not available");
            MLE_W("Validation layers are disabled");
        }
    }

    MLE_D("Enabled extensions:");
    for (const auto* e : instance_extensions) {
        MLE_D("  {}", e);
    }

    MLE_D("Enabled layers:");
    for (const auto* e : instance_layers) {
        MLE_D("  {}", e);
    }

    instance_ci.pApplicationInfo = &app_info;
    instance_ci.setPEnabledLayerNames(instance_layers);
    instance_ci.setPEnabledExtensionNames(instance_extensions);

    auto create_instance_r = vk::createInstance(instance_ci);
    if (create_instance_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to create Vulkan instance! Error code: {}", create_instance_r.result);
        return Result::VK_ERROR;
    }
    vk_instance_ = create_instance_r.value;
    shutdown_delete_queue_.emplace_back([this]() { vk_instance_.destroy(); });

    return Result::OK;
}

Result Impl::createVkDebugMessenger() {
    MLE_D("Creating Vulkan debug messenger");

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    auto create_r = vk_instance_.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{{}, severity_flags, message_type_flags, &debugCallback});
    if (create_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to create Vulkan debug messenger! Error code: {}", create_r.result);
        return Result::VK_ERROR;
    }
    debug_messenger_ = create_r.value;
    shutdown_delete_queue_.emplace_back([this]() { vk_instance_.destroyDebugUtilsMessengerEXT(debug_messenger_); });

    return Result::OK;
}

Result Impl::createVkSurface() {
    MLE_D("Creating Vulkan surface");

    VkSurfaceKHR temp = VK_NULL_HANDLE;
    auto create_r = glfwCreateWindowSurface(vk_instance_, window::getGlfwWindowRef(), nullptr, &temp);
    if (create_r != VK_SUCCESS) {
        MLE_E("Failed to create surface! Error code: {}", vk::Result(create_r));
        return Result::VK_ERROR;
    }
    surface_ = temp;
    shutdown_delete_queue_.emplace_back([this]() { vk_instance_.destroySurfaceKHR(surface_); });

    return Result::OK;
}

Result Impl::createVkDevice() {
    MLE_D("Creating Vulkan device");

    if (isError(pickPhysicalDevice())) {
        MLE_E("Failed to pick a suitable physical device!");
        return Result::NOK;
    }

    auto pick_depth_r = pickFormat({vk::Format::eD24UnormS8Uint}, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    if (!pick_depth_r) {
        MLE_E("Failed to pick a suitable depth format! Error: {}", pick_depth_r.error());
        return Result::NOK;
    }
    depth_format_ = pick_depth_r.value();

    std::vector target_color_formats{vk::Format::eB8G8R8A8Srgb};
    vk::FormatFeatureFlags target_color_flags = vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc |
                                                vk::FormatFeatureFlagBits::eTransferDst | vk::FormatFeatureFlagBits::eSampledImage |
                                                vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;
    auto pick_color_r = pickFormat(target_color_formats, target_color_flags);
    if (!pick_color_r) {
        MLE_E("Failed to pick a suitable color format! Error: {}", pick_color_r.error());
        return Result::NOK;
    }
    color_format_ = pick_color_r.value();

    if (isError(pickAioQueueIdx())) {
        MLE_E("Failed to pick queue indices!");
        return Result::NOK;
    }

    f32 queue_priority = 1;
    std::array<vk::DeviceQueueCreateInfo, 1> queue_cis;
    auto& aio_queue_ci = queue_cis[0];
    aio_queue_ci.queueFamilyIndex = aio_queue_idx_;
    aio_queue_ci.pQueuePriorities = &queue_priority;
    aio_queue_ci.queueCount = 1;

    // TODO: this
    // auto& transfer_queue_ci = queue_cis[1];
    // transfer_queue_ci.queueFamilyIndex = transfer_queue_idx_;
    // transfer_queue_ci.pQueuePriorities = &queue_priority;
    // transfer_queue_ci.queueCount = 1;

    vk::PhysicalDeviceHostQueryResetFeatures host_query_reset_feature{};
    host_query_reset_feature.hostQueryReset = vk::True;

    vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_feature{};
    descriptor_indexing_feature.pNext = &host_query_reset_feature;
    descriptor_indexing_feature.runtimeDescriptorArray = vk::True;

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features{};
    extended_dynamic_state_features.pNext = &descriptor_indexing_feature;
    extended_dynamic_state_features.extendedDynamicState = vk::True;

    vk::PhysicalDeviceVulkan13Features vulkan13_features{};
    vulkan13_features.pNext = &extended_dynamic_state_features;
    vulkan13_features.synchronization2 = vk::True;
    vulkan13_features.dynamicRendering = vk::True;

    vk::PhysicalDeviceFeatures2 required_features{};
    required_features.pNext = &vulkan13_features;
    required_features.features.wideLines = vk::True;

    std::array required_extensions{vk::KHRSwapchainExtensionName, vk::KHRPushDescriptorExtensionName};

    vk::DeviceCreateInfo device_ci{};
    device_ci.pNext = &required_features;
    device_ci.queueCreateInfoCount = 1;
    device_ci.setQueueCreateInfos(queue_cis);
    device_ci.setPEnabledExtensionNames(required_extensions);

    auto create_device_r = p_device_.device.createDevice(device_ci);
    if (create_device_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to create Vulkan device! Error code: {}", create_device_r.result);
        return Result::VK_ERROR;
    }
    device_ = create_device_r.value;
    shutdown_delete_queue_.emplace_back([this]() { device_.destroy(); });

    aio_queue_ = device_.getQueue(aio_queue_idx_, 0);

    logDevice();

    return Result::OK;
}

Result Impl::pickPhysicalDevice() {
    MLE_D("Picking physical device");

    auto physical_devices_r = vk_instance_.enumeratePhysicalDevices();
    if (physical_devices_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to enumerate physical devices! Error code: {}", physical_devices_r.result);
        return Result::VK_ERROR;
    }
    auto& physical_devices = physical_devices_r.value;

    MLE_T("Found {} devices that know Vulkan.", physical_devices.size());
    for (auto i : physical_devices) {
        MLE_T("  {}", i.getProperties().deviceName.data());
    }

    vk::PhysicalDevice chosen_device;

    // TODO: this
    MLE_T("TODO: For now picking the first discrete GPU available that supports Vulkan.");
    for (auto i : physical_devices) {
        if (i.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            chosen_device = i;
            break;
        }
    }

    if (!chosen_device) {
        MLE_E("No suitable physical device found! Please ensure you have a Vulkan-compatible GPU and drivers installed.");
        return Result::VK_ERROR;
    }

    p_device_.device = chosen_device;
    p_device_.properties = p_device_.device.getProperties();
    p_device_.features = p_device_.device.getFeatures();
    p_device_.memory_properties = p_device_.device.getMemoryProperties();
    p_device_.queue_family_properties = p_device_.device.getQueueFamilyProperties();

    MLE_I("Chosen device: {}", p_device_.device.getProperties().deviceName.data());

    return Result::OK;
}

Expected<vk::Format> Impl::pickFormat(const std::vector<vk::Format>& candidates, vk::FormatFeatureFlags flags) const {
    for (const auto& candidate : candidates) {
        vk::FormatProperties format_properties = p_device_.device.getFormatProperties(candidate);

        if (format_properties.optimalTilingFeatures & flags) {
            return candidate;
        }
    }

    return std::unexpected(Result::NOT_FOUND);
}

Result Impl::pickAioQueueIdx() {
    MLE_T("Picking aio queue index");

    for (usize i = 0; i < p_device_.queue_family_properties.size(); ++i) {
        auto flags = p_device_.queue_family_properties[i].queueFlags;
        auto q_count = p_device_.queue_family_properties[i].queueCount;

        bool has_graphics = !!(flags & vk::QueueFlagBits::eGraphics);
        bool has_transfer = !!(flags & vk::QueueFlagBits::eTransfer);
        bool has_compute = !!(flags & vk::QueueFlagBits::eCompute);
        bool surface_support = p_device_.device.getSurfaceSupportKHR(i, surface_).result == vk::Result::eSuccess;

        if (has_graphics && has_transfer && has_compute && surface_support) {
            MLE_T("Found suitable AIO queue index: {}, q_count: {}", i, q_count);
            aio_queue_idx_ = i;
            break;
        }
    }

    if (aio_queue_idx_ == max<usize>()) {
        MLE_E("Failed to find a suitable AIO queue index!");
        return Result::NOT_FOUND;
    }

    return Result::OK;
}

Result Impl::createVMA() {
    MLE_D("Creating Vulkan Memory Allocator (VMA)");

    VmaAllocatorCreateInfo vma_ci{};
    vma_ci.physicalDevice = p_device_.device;
    vma_ci.device = device_;
    vma_ci.instance = vk_instance_;
    vma_ci.vulkanApiVersion = vk::ApiVersion13;

    auto vma_r = vmaCreateAllocator(&vma_ci, &vma_);
    if (vma_r != VK_SUCCESS) {
        MLE_E("Failed to create Vulkan Memory Allocator! Error code: {}", vk::Result(vma_r));
        return Result::VK_ERROR;
    }
    shutdown_delete_queue_.emplace_back([this]() { vmaDestroyAllocator(vma_); });

    return Result::OK;
}

void Impl::logDevice() {
    MLE_I("Selected physical device {}.", p_device_.properties.deviceName.data());
    MLE_I("Device type: {}", vk::to_string(p_device_.properties.deviceType));
    MLE_I("Driver version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.driverVersion), VK_VERSION_MINOR(p_device_.properties.driverVersion),
          VK_VERSION_PATCH(p_device_.properties.driverVersion));
    MLE_I("Vulkan API version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.apiVersion), VK_VERSION_MINOR(p_device_.properties.apiVersion),
          VK_VERSION_PATCH(p_device_.properties.apiVersion));
    MLE_I("Queue indices: AIO: {}", aio_queue_idx_);
    MLE_I("Depth format: {}", vk::to_string(depth_format_));
    MLE_I("Color format: {}", vk::to_string(color_format_));
    MLE_I("Memory properties:");
    MLE_I("  Memory types: {}", p_device_.memory_properties.memoryTypeCount);
    for (u32 i = 0; i < p_device_.memory_properties.memoryTypeCount; i++) {
        MLE_I("  Type({}) flags: {}, heap index: {}", i, vk::to_string(p_device_.memory_properties.memoryTypes[i].propertyFlags),  // NOLINT
              p_device_.memory_properties.memoryTypes[i].heapIndex);                                                               // NOLINT
    }
    MLE_I("  Memory heaps: {}", p_device_.memory_properties.memoryHeapCount);
    for (u32 i = 0; i < p_device_.memory_properties.memoryHeapCount; i++) {
        MLE_I("  Heap({}) {}: {}MiB", i, vk::to_string(p_device_.memory_properties.memoryHeaps[i].flags),  // NOLINT
              p_device_.memory_properties.memoryHeaps[i].size / 1024 / 1024);                              // NOLINT
    }
}

void Impl::shutdown() {
    MLE_I("Shutting down Renderer");

    if (device_) {
        waitIdle();

        if (swapchain_) {
            MLE_D("Destroying swapchain");
            device_.destroy(swapchain_);
            swapchain_ = nullptr;
        }
    }

    for (auto& it : std::ranges::reverse_view(shutdown_delete_queue_)) {
        it();
    }
}

void Impl::waitIdle() {
    MLE_ASSERT(device_);
    MLE_D("Waiting for device to become idle.");
    auto r = device_.waitIdle();
    MLE_ASSERT_LOG(r == vk::Result::eSuccess, "Failed to wait for device to become idle! Error code: {}", r);
}

vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats, vk::SurfaceFormatKHR target_format) {
    for (const auto& format : available_formats) {
        if (format.format == target_format.format && format.colorSpace == target_format.colorSpace) {
            return format;
        }
    }

    return available_formats[0];
}

Result Impl::createSwapchain() {
    MLE_D("Creating swapchain");

    auto old_swapchain = swapchain_;

    auto surface_capabilities_r = p_device_.device.getSurfaceCapabilitiesKHR(surface_);
    if (surface_capabilities_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to get surface capabilities! Error code: {}", surface_capabilities_r.result);
        return Result::VK_ERROR;
    }
    auto surface_formats_r = p_device_.device.getSurfaceFormatsKHR(surface_);
    if (surface_formats_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to get surface formats! Error code: {}", surface_formats_r.result);
        return Result::VK_ERROR;
    }
    auto surface_present_modes_r = p_device_.device.getSurfacePresentModesKHR(surface_);
    if (surface_present_modes_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to get surface present modes! Error code: {}", surface_present_modes_r.result);
        return Result::VK_ERROR;
    }

    auto& surface_capabilities = surface_capabilities_r.value;
    auto& surface_formats = surface_formats_r.value;
    [[maybe_unused]] auto& surface_present_modes = surface_present_modes_r.value;

    auto swapchain_extent = surface_capabilities.currentExtent;
    auto surface_format = chooseSwapchainSurfaceFormat(surface_formats, vk::SurfaceFormatKHR{color_format_, vk::ColorSpaceKHR::eSrgbNonlinear});

#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
    auto surface_present_mode = runtime_config::getPresentMode();
#else
    auto surface_present_mode = vk::PresentModeKHR::eFifo;
#endif

    usize image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_ci{};
    swapchain_ci.surface = surface_;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = surface_format.format;
    swapchain_ci.imageColorSpace = surface_format.colorSpace;
    swapchain_ci.imageExtent = swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = vk::ImageUsageFlagBits::eTransferDst;
    swapchain_ci.preTransform = surface_capabilities.currentTransform;
    swapchain_ci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchain_ci.presentMode = surface_present_mode;
    swapchain_ci.clipped = vk::True;
    swapchain_ci.oldSwapchain = old_swapchain;
    swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;

    auto swapchain_create_r = device_.createSwapchainKHR(swapchain_ci);
    if (swapchain_create_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to create swapchain! Error code: {}", swapchain_create_r.result);
        return Result::VK_ERROR;
    }
    swapchain_ = swapchain_create_r.value;

    if (old_swapchain) {
        swapchain_images_.clear();
        device_.destroy(old_swapchain);
    }

    auto swapchain_images_r = device_.getSwapchainImagesKHR(swapchain_);
    if (swapchain_images_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to get swapchain images! Error code: {}", swapchain_images_r.result);
        return Result::VK_ERROR;
    }
    swapchain_images_ = std::move(swapchain_images_r.value);

    swapchain_format_ = surface_format.format;
    swapchain_extent_ = swapchain_extent;

    MLE_I("Swapchain created. Image count:{} Present mode:{} Surface format:[{}, {}] Extent:{}", swapchain_images_.size(),
          vk::to_string(swapchain_ci.presentMode), vk::to_string(surface_format.format), vk::to_string(surface_format.colorSpace),
          vec2u{swapchain_extent.width, swapchain_extent.height});

    return Result::OK;
}
}  // namespace

Result init([[maybe_unused]] const CI& ci) {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    auto result = i_->init();
    if (result != Result::OK) {
        MLE_C("Renderer initialization failed with error: {}", result);
        i_.reset();
    }
    return result;
}

void shutdown() {
    if (i_) {
        MLE_I("Shutting down Renderer");
        i_->shutdown();
        i_.reset();
    }
}
}  // namespace mle::renderer
