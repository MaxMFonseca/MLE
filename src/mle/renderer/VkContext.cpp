#include "VkContext.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "GLFW/glfw3.h"
#include "mle/common/Exception.h"
#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/window/Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace mle::renderer {
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

bool isValidationLayersPresent() {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    MLE_T("Available layers:");
    for (const auto& layer_properties : available_layers) {
        MLE_T("  {}", layer_properties.layerName);
    }

    std::array required_layers = {"VK_LAYER_KHRONOS_validation"};

    for (const char* layer_name : required_layers) {
        bool layer_found = false;
        for (const auto& layer_properties : available_layers) {
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

vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats, vk::SurfaceFormatKHR target_format) {
    for (const auto& format : available_formats) {
        if (format.format == target_format.format && format.colorSpace == target_format.colorSpace) {
            return format;
        }
    }

    return available_formats[0];
}
}  // namespace

void VkContext::init() {
    MLE_I("Initializing Vulkan context");

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    createInstance();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);

    if constexpr (IS_DEBUG_BUILD) {
        createDebugMessenger();
    }

    createSurface();
    createDevice();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);

    createVma();
}

void VkContext::shutdown() {
    MLE_I("Shuting down Vulkan context");

    waitIdle();

    if (swapchain_) {
        MLE_T("swapchain");
        device_.destroy(swapchain_);
    }

    MLE_T("vma");
    if constexpr (IS_DEBUG_BUILD) {
        char* stats_string = nullptr;
        vmaBuildStatsString(vma_, &stats_string, 1U);
        MLE_T("VMA stats before shutdown:\n{}", stats_string);
        vmaFreeStatsString(vma_, stats_string);
    }
    vmaDestroyAllocator(vma_);

    MLE_T("device");
    device_.destroy();

    MLE_T("surface");
    instance_.destroy(surface_);

    MLE_T("debug messenger");
    if (debug_messenger_) {
        instance_.destroy(debug_messenger_);
    }

    MLE_T("instance");
    instance_.destroy();
}

void VkContext::createInstance() {
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
        if (isValidationLayersPresent()) {
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

    instance_ = vk::createInstance(instance_ci);
}

void VkContext::createDebugMessenger() {
    MLE_LOG_THIS;

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    debug_messenger_ = instance_.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{{}, severity_flags, message_type_flags, &debugCallback});
}

void VkContext::createSurface() {
    MLE_LOG_THIS_D;

    VkSurfaceKHR temp = VK_NULL_HANDLE;
    auto result = glfwCreateWindowSurface(instance_, window::getGlfwWindowRef(), nullptr, &temp);
    if (result != VK_SUCCESS) {
        MLE_E("Failed to create surface! Error code: {}", vk::to_string(vk::Result(result)));
        return;
    }
    surface_ = temp;
}

void VkContext::createDevice() {
    MLE_LOG_THIS_D;

    pickPhysicalDevice();

    physical_device_properties_ = physical_device_.getProperties();
    physical_device_features_ = physical_device_.getFeatures();
    physical_device_memory_properties_ = physical_device_.getMemoryProperties();
    physical_device_queue_family_properties_ = physical_device_.getQueueFamilyProperties();

    pickDepthFormat();
    pickColorFormat();

    pickQueueIdxs();

    logPhysicalDevice();

    MLE_ASSERT(getTimestampPeriod() > 0);

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

    device_ = physical_device_.createDevice(device_ci);

    aio_queue_ = device_.getQueue(aio_queue_idx_, 0);
    transfer_queue_ = device_.getQueue(transfer_queue_idx_, 0);
}

void VkContext::createVma() {
    MLE_LOG_THIS_D;

    VmaAllocatorCreateInfo vma_ci{};
    vma_ci.physicalDevice = physical_device_;
    vma_ci.device = device_;
    vma_ci.instance = instance_;
    vma_ci.vulkanApiVersion = vk::ApiVersion13;

    auto vma_r = vmaCreateAllocator(&vma_ci, &vma_);

    if (vma_r != VK_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to create VMA allocator! Error code: {}", vk::to_string((vk::Result)vma_r));
    }
}

void VkContext::createSwapchain() {
    MLE_LOG_THIS_D;

    auto old_swapchain = swapchain_;

    auto surface_capabilities = physical_device_.getSurfaceCapabilitiesKHR(surface_);
    auto surface_formats = physical_device_.getSurfaceFormatsKHR(surface_);
    auto surface_present_modes = physical_device_.getSurfacePresentModesKHR(surface_);

    auto swapchain_extent = surface_capabilities.currentExtent;
    auto surface_format = chooseSwapchainSurfaceFormat(surface_formats, vk::SurfaceFormatKHR{color_format_, vk::ColorSpaceKHR::eSrgbNonlinear});

#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
    auto surface_present_mode = runtime_config::getPresentMode();
#else
    auto surface_present_mode = vk::PresentModeKHR::eFifo;
#endif

    u32 image_count = surface_capabilities.minImageCount + 1;
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

    swapchain_ = device_.createSwapchainKHR(swapchain_ci);

    if (old_swapchain) {
        swapchain_images_.clear();
        device_.destroy(old_swapchain);
    }

    swapchain_images_ = device_.getSwapchainImagesKHR(swapchain_);

    swapchain_format_ = surface_format.format;
    swapchain_extent_ = swapchain_extent;

    MLE_I("Swapchain info: Image count:{} Present mode:{} Surface format:[{}, {}] Extent:{}", swapchain_images_.size(), vk::to_string(swapchain_ci.presentMode),
          vk::to_string(surface_format.format), vk::to_string(surface_format.colorSpace), vec2u{swapchain_extent.width, swapchain_extent.height});
}

// TODO: this
void VkContext::pickPhysicalDevice() {
    MLE_LOG_THIS_T;

    auto physical_devices = instance_.enumeratePhysicalDevices();

    MLE_T("Found {} devices that know Vulkan.", physical_devices.size());
    for (auto i : physical_devices) {
        MLE_T("  {}", i.getProperties().deviceName);
    }

    for (auto i : physical_devices) {
        if (i.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            physical_device_ = i;
            return;
        }
    }

    MLE_THROW(VK_ERROR, "No suitable physical device found!");
}

void VkContext::pickDepthFormat() {
    MLE_LOG_THIS_T;

    // TODO: this
    std::array candidates = {vk::Format::eD24UnormS8Uint};
    vk::FormatFeatureFlags flags = vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    for (auto candidate : candidates) {
        vk::FormatProperties format_properties = physical_device_.getFormatProperties(candidate);

        if (format_properties.linearTilingFeatures & flags || format_properties.optimalTilingFeatures & flags) {
            depth_format_ = candidate;
            MLE_D("Picked depth format: {}", vk::to_string(depth_format_));
            return;
        }
    }

    MLE_THROW(VK_ERROR, "Failed to pick a depth format!");
}

void VkContext::pickColorFormat() {
    MLE_LOG_THIS_T;

    std::array candidates = {vk::Format::eB8G8R8A8Srgb};
    vk::FormatFeatureFlags flags = vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc |
                                   vk::FormatFeatureFlagBits::eTransferDst | vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eBlitSrc |
                                   vk::FormatFeatureFlagBits::eBlitDst;

    for (auto candidate : candidates) {
        vk::FormatProperties format_properties = physical_device_.getFormatProperties(candidate);

        if (format_properties.optimalTilingFeatures & flags) {
            color_format_ = candidate;
            MLE_D("Picked color format: {}", vk::to_string(color_format_));
            return;
        }
    }

    MLE_THROW(VK_ERROR, "Failed to pick a color format!");
}

void VkContext::pickQueueIdxs() {
    MLE_T("Picking mono queue index...");

    for (usize i = 0; i < physical_device_queue_family_properties_.size(); ++i) {
        auto flags = physical_device_queue_family_properties_[i].queueFlags;

        if (flags & (vk::QueueFlagBits::eGraphics) && flags & (vk::QueueFlagBits::eTransfer) && flags & (vk::QueueFlagBits::eCompute) &&
            physical_device_.getSurfaceSupportKHR(i, surface_) == vk::True) {
            aio_queue_idx_ = i;
        }
        if (flags & vk::QueueFlagBits::eTransfer && !(flags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eOpticalFlowNV |
                                                               vk::QueueFlagBits::eVideoEncodeKHR | vk::QueueFlagBits::eVideoDecodeKHR))) {
            transfer_queue_idx_ = i;
        }
    }

    if (aio_queue_idx_ == max<usize>()) {
        MLE_THROW(VK_ERROR, "AIO queue not found!");
    }
    if (transfer_queue_idx_ == max<usize>()) {
        transfer_queue_idx_ = aio_queue_idx_;
        MLE_I("Transfer queue not found, using AIO queue for transfer.");
    }

    // TODO: this
    transfer_queue_idx_ = aio_queue_idx_;
}

void VkContext::logPhysicalDevice() {
    MLE_I("Selected physical device {}.", physical_device_properties_.deviceName);

    MLE_I("Driver version: {}.{}.{}", VK_VERSION_MAJOR(physical_device_properties_.driverVersion), VK_VERSION_MINOR(physical_device_properties_.driverVersion),
          VK_VERSION_PATCH(physical_device_properties_.driverVersion));
    MLE_I("Vulkan API version: {}.{}.{}", VK_VERSION_MAJOR(physical_device_properties_.apiVersion), VK_VERSION_MINOR(physical_device_properties_.apiVersion),
          VK_VERSION_PATCH(physical_device_properties_.apiVersion));

    MLE_I("Queue indices: AIO: {}, Transfer: {}", aio_queue_idx_, transfer_queue_idx_);

    MLE_I("Depth format: {}", vk::to_string(depth_format_));

    MLE_I("Memory heaps:");
    for (u32 i = 0; i < physical_device_memory_properties_.memoryHeapCount; i++) {
        MLE_I("  Heap({}) {}: {}MiB", i, vk::to_string(physical_device_memory_properties_.memoryHeaps[i].flags),  // NOLINT
              physical_device_memory_properties_.memoryHeaps[i].size / 1024 / 1024);                              // NOLINT
    }
}

void VkContext::waitIdle() {
    MLE_LOG_THIS;
    device_.waitIdle();
}
}  // namespace mle::renderer
