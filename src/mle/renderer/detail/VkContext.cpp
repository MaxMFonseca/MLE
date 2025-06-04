#include "VkContext.h"

#include <vulkan/vulkan_core.h>

#include "GLFW/glfw3.h"
#include "mle/common/Logger.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"
#include "mle/window/Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace mle::renderer::detail {
namespace {
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
}  // namespace

void VkContext::init() {
    MLE_I("Initializing Vulkan context");

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    initInstance();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance_);

    if constexpr (IS_DEBUG_BUILD) {
        initDebugMessenger();
    }

    initSurface();
    initDevice();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);

    initVMA();
}

void VkContext::waitIdle() {
    if (!device_) {
        MLE_W("Vulkan device is not initialized, cannot wait for idle.");
        return;
    }

    MLE_I("Waiting for Vulkan device to become idle");
    auto r = device_.waitIdle();
    if (r != vk::Result::eSuccess) {
        core::unrecoverable("Failed to wait for Vulkan device to become idle! Error code: {}", r);
    }
}

void VkContext::initInstance() {
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
        core::unrecoverable("Failed to create Vulkan instance! Error code: {}", create_instance_r.result);
    }

    vk_instance_ = create_instance_r.value;
    renderer::addOnShutdown([this]() { vk_instance_.destroy(); });
}

void VkContext::initDebugMessenger() {
    MLE_D("Creating Vulkan debug messenger");

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    auto create_r = vk_instance_.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{{}, severity_flags, message_type_flags, &debugCallback});
    if (create_r.result != vk::Result::eSuccess) {
        MLE_E("Failed to create Vulkan debug messenger! Error code: {}", create_r.result);
        return;
    }
    debug_messenger_ = create_r.value;
    renderer::addOnShutdown([this]() { vk_instance_.destroy(debug_messenger_); });
}

void VkContext::initSurface() {
    MLE_D("Creating Vulkan surface");

    VkSurfaceKHR temp = VK_NULL_HANDLE;
    auto create_r = glfwCreateWindowSurface(vk_instance_, window::getGlfwWindowRef(), nullptr, &temp);
    if (create_r != VK_SUCCESS) {
        core::unrecoverable("Failed to create GLFW window surface! Error code: {}", as<vk::Result>(create_r));
    }
    surface_ = temp;

    renderer::addOnShutdown([this]() { vk_instance_.destroy(surface_); });
}

void VkContext::initDevice() {
    MLE_D("Creating Vulkan device");

    pickPhysicalDevice();

    auto pick_depth_r = pickFormat({vk::Format::eD24UnormS8Uint}, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    if (!pick_depth_r) {
        core::unrecoverable("Failed to pick a suitable depth format! Error: {}", pick_depth_r.error());
    }
    depth_format_ = pick_depth_r.value();

    std::vector target_color_formats{vk::Format::eB8G8R8A8Srgb};
    vk::FormatFeatureFlags target_color_flags = vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc |
                                                vk::FormatFeatureFlagBits::eTransferDst | vk::FormatFeatureFlagBits::eSampledImage |
                                                vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;
    auto pick_color_r = pickFormat(target_color_formats, target_color_flags);
    if (!pick_color_r) {
        core::unrecoverable("Failed to pick a suitable color format! Error: {}", pick_color_r.error());
    }
    color_format_ = pick_color_r.value();

    pickQueueIndices();

    std::array queue_priority{1.F, 0.5F, 0.5F, 0.5F};
    int graphics_queue_count = 2;

    std::vector<vk::DeviceQueueCreateInfo> queue_cis;
    if (queue_indices_.at(as<usize>(CmdType::TRANSFER)) != max<usize>()) {
        vk::DeviceQueueCreateInfo t_queue_ci{};
        t_queue_ci.queueFamilyIndex = queue_indices_.at(as<usize>(CmdType::TRANSFER));
        t_queue_ci.pQueuePriorities = &queue_priority[1];
        t_queue_ci.queueCount = 1;
        queue_cis.push_back(t_queue_ci);
    } else {
        graphics_queue_count += 1;
    }

    if (queue_indices_.at(as<usize>(CmdType::COMPUTE)) != max<usize>()) {
        vk::DeviceQueueCreateInfo c_queue_ci{};
        c_queue_ci.queueFamilyIndex = queue_indices_.at(as<usize>(CmdType::COMPUTE));
        c_queue_ci.pQueuePriorities = &queue_priority[1];
        c_queue_ci.queueCount = 1;
        queue_cis.push_back(c_queue_ci);
    } else {
        graphics_queue_count += 1;
    }

    vk::DeviceQueueCreateInfo g_queue_ci{};
    g_queue_ci.queueFamilyIndex = queue_indices_.at(as<usize>(CmdType::GRAPHICS));
    g_queue_ci.pQueuePriorities = queue_priority.data();
    g_queue_ci.queueCount = graphics_queue_count;
    queue_cis.push_back(g_queue_ci);

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
        core::unrecoverable("Failed to create Vulkan device! Error code: {}", create_device_r.result);
    }
    device_ = create_device_r.value;
    renderer::addOnShutdown([this]() { device_.destroy(); });

    logDevice();

    render_queue_ = device_.getQueue(queue_indices_.at(as<usize>(CmdType::GRAPHICS)), 0);
}

std::array<vk::Queue, 3> VkContext::getDedicatedQueues() {
    int g_queue_idx = 1;

    std::array<vk::Queue, 3> ret;

    ret.at(as<usize>(CmdType::GRAPHICS)) = device_.getQueue(queue_indices_.at(as<usize>(CmdType::GRAPHICS)), g_queue_idx);
    g_queue_idx++;

    if (queue_indices_.at(as<usize>(CmdType::TRANSFER)) == queue_indices_.at(as<usize>(CmdType::GRAPHICS))) {
        ret.at(as<usize>(CmdType::TRANSFER)) = device_.getQueue(queue_indices_.at(as<usize>(CmdType::GRAPHICS)), g_queue_idx);
        g_queue_idx++;
    } else {
        ret.at(as<usize>(CmdType::TRANSFER)) = device_.getQueue(queue_indices_.at(as<usize>(CmdType::TRANSFER)), 0);
    }

    if (queue_indices_.at(as<usize>(CmdType::COMPUTE)) == queue_indices_.at(as<usize>(CmdType::GRAPHICS))) {
        ret.at(as<usize>(CmdType::COMPUTE)) = device_.getQueue(queue_indices_.at(as<usize>(CmdType::GRAPHICS)), g_queue_idx);
        g_queue_idx++;
    } else {
        ret.at(as<usize>(CmdType::COMPUTE)) = device_.getQueue(queue_indices_.at(as<usize>(CmdType::COMPUTE)), 0);
    }

    return ret;
}

void VkContext::pickPhysicalDevice() {
    MLE_D("Picking physical device");

    auto physical_devices_r = vk_instance_.enumeratePhysicalDevices();
    if (physical_devices_r.result != vk::Result::eSuccess) {
        core::unrecoverable("Failed to enumerate physical devices! Error code: {}", physical_devices_r.result);
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
        core::unrecoverable("No discrete GPU found that supports Vulkan! Please ensure you have a Vulkan-compatible GPU and drivers installed.");
    }

    p_device_.device = chosen_device;
    p_device_.properties = p_device_.device.getProperties();
    p_device_.features = p_device_.device.getFeatures();
    p_device_.memory_properties = p_device_.device.getMemoryProperties();
    p_device_.queue_family_properties = p_device_.device.getQueueFamilyProperties();

    MLE_I("Chosen device: {}", p_device_.device.getProperties().deviceName.data());
}

Expected<vk::Format> VkContext::pickFormat(const std::vector<vk::Format>& candidates, vk::FormatFeatureFlags flags) const {
    for (const auto& candidate : candidates) {
        vk::FormatProperties format_properties = p_device_.device.getFormatProperties(candidate);

        if (format_properties.optimalTilingFeatures & flags) {
            return candidate;
        }
    }

    return std::unexpected(Result::NOT_FOUND);
}

void VkContext::pickQueueIndices() {
    MLE_T("Picking graphics, compute, and transfer queue indices");

    const auto& families = p_device_.queue_family_properties;
    auto graphics_idx = max<usize>();
    auto compute_idx = max<usize>();
    auto transfer_idx = max<usize>();

    for (usize i = 0; i < families.size(); ++i) {
        auto flags = families[i].queueFlags;
        auto q_count = families[i].queueCount;

        const bool supports_graphics = !!(flags & vk::QueueFlagBits::eGraphics);
        const bool supports_compute = !!(flags & vk::QueueFlagBits::eCompute);
        const bool supports_transfer = !!(flags & vk::QueueFlagBits::eTransfer);
        const bool supports_surface = supports_graphics && (as<bool>(p_device_.device.getSurfaceSupportKHR(i, surface_).value));

        const bool is_special = as<VkQueueFlags>(flags) > VK_QUEUE_VIDEO_DECODE_BIT_KHR;
        if (is_special) {
            break;
        }

        if (supports_surface && graphics_idx == max<usize>()) {
            graphics_idx = i;
            MLE_T("Graphics queue index: {}, count: {}", i, q_count);
        }

        if (supports_compute && !supports_graphics && compute_idx == max<usize>()) {
            compute_idx = i;
            MLE_T("Dedicated compute queue index: {}, count: {}", i, q_count);
        }

        if (supports_transfer && !supports_graphics && !supports_compute && transfer_idx == max<usize>()) {
            transfer_idx = i;
            MLE_T("Dedicated transfer queue index: {}, count: {}", i, q_count);
        }
    }

    if (graphics_idx == max<usize>()) {
        core::unrecoverable("Failed to find a suitable graphics queue index!");
    }

    if (compute_idx == max<usize>()) {
        compute_idx = graphics_idx;
        MLE_T("No dedicated compute queue found, using graphics queue index: {}", compute_idx);
    }

    if (transfer_idx == max<usize>()) {
        transfer_idx = graphics_idx;
        MLE_T("No dedicated transfer queue found, using graphics queue index: {}", transfer_idx);
    }

    queue_indices_.at(as<usize>(CmdType::GRAPHICS)) = graphics_idx;
    queue_indices_.at(as<usize>(CmdType::COMPUTE)) = compute_idx;
    queue_indices_.at(as<usize>(CmdType::TRANSFER)) = transfer_idx;
}

void VkContext::initVMA() {
    MLE_D("Creating Vulkan Memory Allocator (VMA)");

    VmaAllocatorCreateInfo vma_ci{};
    vma_ci.physicalDevice = p_device_.device;
    vma_ci.device = device_;
    vma_ci.instance = vk_instance_;
    vma_ci.vulkanApiVersion = vk::ApiVersion13;

    auto vma_r = vmaCreateAllocator(&vma_ci, &vma_);
    if (vma_r != VK_SUCCESS) {
        core::unrecoverable("Failed to create Vulkan Memory Allocator! Error code: {}", vk::Result(vma_r));
    }

    renderer::addOnShutdown([this]() { vmaDestroyAllocator(vma_); });
}

void VkContext::logDevice() {
    MLE_I("Selected physical device {}.", p_device_.properties.deviceName.data());
    MLE_I("Device type: {}", vk::to_string(p_device_.properties.deviceType));
    MLE_I("Driver version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.driverVersion), VK_VERSION_MINOR(p_device_.properties.driverVersion),
          VK_VERSION_PATCH(p_device_.properties.driverVersion));
    MLE_I("Vulkan API version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.apiVersion), VK_VERSION_MINOR(p_device_.properties.apiVersion),
          VK_VERSION_PATCH(p_device_.properties.apiVersion));
    MLE_I("Queue indices: g: {}, t: {}, c: {}", queue_indices_.at(as<usize>(CmdType::GRAPHICS)), queue_indices_.at(as<usize>(CmdType::TRANSFER)),
          queue_indices_.at(as<usize>(CmdType::COMPUTE)));
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

}  // namespace mle::renderer::detail
