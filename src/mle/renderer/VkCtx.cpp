#include "VkCtx.h"

#include <fmt/ranges.h>
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan_structs.hpp>

#include "Buffer.h"
#include "Image.h"
#include "Utils.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/core/Unwrap.h"
#include "mle/renderer/Types.h"
#include "mle/utils/ECS.h"
#include "mle/utils/String.h"
#include "mle/window/Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace mle {
namespace {
[[maybe_unused]] VkBool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity, vk::DebugUtilsMessageTypeFlagsEXT message_types,
                                                   vk::DebugUtilsMessengerCallbackDataEXT const* callback_data, [[maybe_unused]] void* _) {
    std::stringstream formatted_msg;

    formatted_msg << vk::to_string(message_severity) << ": ";
    formatted_msg << vk::to_string(message_types) << ":\n";
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
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            MLE_E("{}", formatted_msg.str());
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            MLE_W("{}", formatted_msg.str());
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            MLE_I("{}", formatted_msg.str());
            break;
        default:
            MLE_T("{}", formatted_msg.str());
            break;
    }

    return VK_FALSE;
}

[[maybe_unused]] bool areValidationLayersPresent() {
    auto available_layers = unwrap(vk::enumerateInstanceLayerProperties());
    MLE_T("Available layers:");
    for (const auto& layer_properties : available_layers) {
        MLE_T("  {}", layer_properties.layerName.data());
    }

    constexpr std::array REQUIRED_LAYERS = {"VK_LAYER_KHRONOS_validation"};

    for (const char* layer_name : REQUIRED_LAYERS) {
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
}  // namespace

void VkCtx::init() {
    MLE_I("Initializing Vulkan context");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    initInstance();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance_);
    initDebugMessenger();
    initSurface();
    initDevice();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);
    initVMA();

    MLE_I("Vulkan context initialized");
    logDevice();
}

void VkCtx::shutdown() {
    if (!vk_instance_) {
        return;
    }

    MLE_I("Shutting down Vulkan context");

    if (device_) {
        check(device_.waitIdle());
    }

    if (vma_) {
        VmaTotalStatistics stats{};
        vmaCalculateStatistics(vma_, &stats);

        Buffer::logAliveObjects();
        Image::logAliveObjects();

        if (stats.total.statistics.allocationCount > 0) {
            MLE_C("VMA leak detected: {} allocations still active.", stats.total.statistics.allocationCount);

            MLE_C("-------------------");

            MLE_C("-------------------");

            char* stats_string = nullptr;
            vmaBuildStatsString(vma_, &stats_string, VK_TRUE);
            MLE_E("VMA Statistics:\n{}", stats_string);
            vmaFreeStatsString(vma_, stats_string);
        }

        vmaDestroyAllocator(vma_);
        vma_ = nullptr;
    }

    if (device_) {
        device_.destroy();
        device_ = nullptr;
    }

    if (surface_) {
        vk_instance_.destroy(surface_);
        surface_ = nullptr;
    }

    if (debug_messenger_) {
        vk_instance_.destroy(debug_messenger_);
        debug_messenger_ = nullptr;
    }

    if (vk_instance_) {
        vk_instance_.destroy();
        vk_instance_ = nullptr;
    }
}

void VkCtx::initInstance() {
    MLE_D("Creating Vulkan instance");

    auto app_info = vk::ApplicationInfo{};
    app_info.pApplicationName = "Unnamed MLE application!";
    app_info.pEngineName = ENGINE_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> instance_extensions;

    if constexpr (IS_CLIENT) {
        for (const auto* window_extension : Window::getRequiredInstanceExtensions()) {
            instance_extensions.push_back(window_extension);  // NOLINT
        }
    }

    std::vector<const char*> instance_layers;

    vk::InstanceCreateInfo instance_ci{};
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_ci{};

    if constexpr (IS_DEBUG_BUILD) {
        if (areValidationLayersPresent()) {
            instance_extensions.push_back(vk::EXTDebugUtilsExtensionName);
            instance_layers.push_back("VK_LAYER_KHRONOS_validation");

            debug_messenger_ci.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            debug_messenger_ci.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
            debug_messenger_ci.pfnUserCallback = debugCallback;
            instance_ci.pNext = &debug_messenger_ci;
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

    vk_instance_ = unwrap(vk::createInstance(instance_ci));
}

void VkCtx::initDebugMessenger() {
    MLE_D("Creating Vulkan debug messenger");

    vk::DebugUtilsMessengerCreateInfoEXT debug_ci;
    debug_ci.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debug_ci.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    debug_ci.pfnUserCallback = debugCallback;

    debug_messenger_ = unwrap(vk_instance_.createDebugUtilsMessengerEXT(debug_ci));
}

void VkCtx::initSurface() {
    if constexpr (!IS_CLIENT) {
        return;
    }

    MLE_D("Creating Vulkan surface");
    surface_ = Window::i().createSurface(vk_instance_);
}

void VkCtx::pickPhysicalDevice() {
    MLE_D("Picking physical device");

    auto physical_devices = unwrap(vk_instance_.enumeratePhysicalDevices());

    MLE_T("Found {} devices that know Vulkan.", physical_devices.size());

    for (auto i : physical_devices) {
        auto properties = i.getProperties();
        auto features = i.getFeatures();

        MLE_T("Device Name: {}", properties.deviceName.data());
        MLE_T("  API Version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
              VK_VERSION_PATCH(properties.apiVersion));
        MLE_T("  Driver Version: {}.{}.{}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion),
              VK_VERSION_PATCH(properties.driverVersion));
        MLE_T("  Vendor ID: {:#06x}", properties.vendorID);
        MLE_T("  Device ID: {:#06x}", properties.deviceID);
        MLE_T("  Device Type: {}", vk::to_string(properties.deviceType));

        if (p_device_.properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu || properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                p_device_.o = i;
                p_device_.properties = properties;
                p_device_.features = features;
            }
        }
    }

    if (p_device_.o == vk::PhysicalDevice{}) {
        MLE_W("No discrete or integrated GPU found, picking the first available device.");
        p_device_.o = physical_devices[0];
        p_device_.properties = p_device_.o.getProperties();
        p_device_.features = p_device_.o.getFeatures();
    }

    p_device_.memory_properties = p_device_.o.getMemoryProperties();
    p_device_.queue_family_properties = p_device_.o.getQueueFamilyProperties();

    MLE_I("Selected physical device {}.", p_device_.properties.deviceName.data());
}

/// NOLINTNEXTLINE(readability-function-cognitive-complexity) its fine
void VkCtx::pickQueueIndices() {
    MLE_D("Picking graphics, compute, and transfer queue indices");

    const auto& families = p_device_.queue_family_properties;

    constexpr auto INVALID = -1;
    auto g_idx = INVALID;
    auto c_idx = INVALID;
    auto t_idx = INVALID;

    auto best_g_score = -1;
    auto best_c_score = -1;
    auto best_t_score = -1;

    const auto family_count = families.size();
    for (usize i = 0; i < family_count; ++i) {
        const auto flags = families[i].queueFlags;
        const auto q_count = families[i].queueCount;

        if (q_count == 0) {
            continue;
        }

        const bool is_special = as<VkQueueFlags>(flags) > VK_QUEUE_VIDEO_DECODE_BIT_KHR;
        if (is_special) {
            continue;
        }

#ifdef MLE_IS_CLIENT
        const bool supports_graphics = !!(flags & vk::QueueFlagBits::eGraphics);
        if (supports_graphics) {
            const auto rv = p_device_.o.getSurfaceSupportKHR(as<u32>(i), surface_);
            bool supports_surface = (rv.result == vk::Result::eSuccess) && as<bool>(rv.value);
            if (supports_surface) {
                int g_score = as<int>(q_count);
                if (g_score > best_g_score) {
                    best_g_score = g_score;
                    g_idx = as<int>(i);
                }
            }
        }
#endif

        const bool supports_compute = !!(flags & vk::QueueFlagBits::eCompute);
        const bool supports_transfer = !!(flags & vk::QueueFlagBits::eTransfer);

        if (supports_compute) {
            int c_score = 0;
            if (!supports_graphics && !supports_transfer) {
                c_score = 300;
            } else if (!supports_graphics) {
                c_score = 250;
            } else {
                c_score = 200;
            }

            c_score += as<int>(q_count);

            if (c_score > best_c_score) {
                best_c_score = c_score;
                c_idx = as<int>(i);
            }
        }

        if (supports_transfer) {
            int t_score = 0;
            if (!supports_graphics && !supports_compute) {
                t_score = 300;
            } else if (!supports_graphics) {
                t_score = 250;
            } else {
                t_score = 200;
            }
            t_score += as<int>(q_count);

            if (t_score > best_t_score) {
                best_t_score = t_score;
                t_idx = as<int>(i);
            }
        }
    }

#ifdef MLE_IS_CLIENT
    queue_data_.g_fam_idx = as<int>(g_idx);
#endif
    queue_data_.c_fam_idx = as<int>(c_idx);
    queue_data_.t_fam_idx = as<int>(t_idx);

    queue_data_.separate_compute = queue_data_.c_fam_idx != queue_data_.g_fam_idx;
    queue_data_.dedicated_transfer = queue_data_.t_fam_idx != queue_data_.g_fam_idx && queue_data_.t_fam_idx != queue_data_.c_fam_idx;

    MLE_D("Selected families:  G:{} C:{} T:{}, separate_compute={}, dedicated_transfer={}", queue_data_.g_fam_idx, queue_data_.c_fam_idx, queue_data_.t_fam_idx,
          queue_data_.separate_compute, queue_data_.dedicated_transfer);
}

namespace {
vk::FormatFeatureFlags2 getOptimalFeatures(vk::PhysicalDevice p_device, vk::Format fmt) {
    vk::FormatProperties3 props3;
    vk::FormatProperties2 props2{{}, &props3};
    p_device.getFormatProperties2(fmt, &props2);
    return props3.optimalTilingFeatures;
}

vk::Format pickOptimalFormat(vk::PhysicalDevice p_device, const auto& candidates, vk::FormatFeatureFlags2 required) {
    for (auto f : candidates) {
        auto feats = getOptimalFeatures(p_device, f);
        if ((feats & required) == required) {
            return f;
        }
    }
    return vk::Format::eUndefined;
}

}  // namespace

void VkCtx::pickImageFormats() {
    MLE_D("Picking preferred formats");

    static constexpr std::array DEPTH{vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint};

    static constexpr std::array TEXTURE4C{vk::Format::eR8G8B8A8Unorm};
    static constexpr std::array TEXTURE4C_SRGB{vk::Format::eR8G8B8A8Srgb};
    static constexpr std::array TEXTURE2C{vk::Format::eR8G8Unorm};
    static constexpr std::array TEXTURE1C{vk::Format::eR8Unorm};

    static constexpr std::array GBUF_PARAMS{vk::Format::eR8G8B8A8Unorm};

    static constexpr std::array NORMALS{vk::Format::eR16G16Sfloat, vk::Format::eR16G16Unorm};

    static constexpr std::array COLOR{vk::Format::eA2B10G10R10UnormPack32, vk::Format::eR16G16B16A16Sfloat, vk::Format::eB8G8R8A8Unorm,
                                      vk::Format::eR8G8B8A8Unorm};
    static constexpr std::array STORAGE_4U8{vk::Format::eR8G8B8A8Unorm};
    static constexpr std::array STORAGE_F32{vk::Format::eR32Sfloat};
    static constexpr std::array STORAGE_U32{vk::Format::eR32Uint};

    static constexpr vk::FormatFeatureFlags2 DEPTH_REQUIRED_FEATURES = vk::FormatFeatureFlagBits2::eDepthStencilAttachment |
                                                                       vk::FormatFeatureFlagBits2::eSampledImage |
                                                                       vk::FormatFeatureFlagBits2::eSampledImageDepthComparison;
    static constexpr vk::FormatFeatureFlags2 TEXTURE_REQUIRED_FEATURES =
        vk::FormatFeatureFlagBits2::eSampledImage | vk::FormatFeatureFlagBits2::eTransferSrc | vk::FormatFeatureFlagBits2::eTransferDst;
    static constexpr vk::FormatFeatureFlags2 GBUF_PARAMS_REQUIRED_FEATURES =
        vk::FormatFeatureFlagBits2::eColorAttachment | vk::FormatFeatureFlagBits2::eSampledImage | vk::FormatFeatureFlagBits2::eTransferSrc |
        vk::FormatFeatureFlagBits2::eTransferDst;
    static constexpr vk::FormatFeatureFlags2 NORMALS_REQUIRED_FEATURES = vk::FormatFeatureFlagBits2::eColorAttachment |
                                                                         vk::FormatFeatureFlagBits2::eSampledImage | vk::FormatFeatureFlagBits2::eTransferSrc |
                                                                         vk::FormatFeatureFlagBits2::eTransferDst;
    static constexpr vk::FormatFeatureFlags2 COLOR_REQUIRED_FEATURES = vk::FormatFeatureFlagBits2::eColorAttachment |
                                                                       vk::FormatFeatureFlagBits2::eSampledImage | vk::FormatFeatureFlagBits2::eTransferSrc |
                                                                       vk::FormatFeatureFlagBits2::eTransferDst | vk::FormatFeatureFlagBits2::eBlitSrc |
                                                                       vk::FormatFeatureFlagBits2::eBlitDst | vk::FormatFeatureFlagBits2::eStorageImage;
    // FIXME: FFS: this is dumb af
    static constexpr vk::FormatFeatureFlags2 STORAGE_REQUIRED_FEATURES =
        vk::FormatFeatureFlagBits2::eStorageImage | vk::FormatFeatureFlagBits2::eTransferSrc | vk::FormatFeatureFlagBits2::eTransferDst;
    static constexpr vk::FormatFeatureFlags2 STORAGE_4U8_REQUIRED_FEATURES = STORAGE_REQUIRED_FEATURES | vk::FormatFeatureFlagBits2::eSampledImage;

    static constexpr vk::ImageUsageFlags DEPTH_USAGE =
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment;
    static constexpr vk::ImageUsageFlags TEXTURE_USAGE =
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    static constexpr vk::ImageUsageFlags GBUF_PARAMS_USAGE = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                                                             vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    static constexpr vk::ImageUsageFlags NORMALS_USAGE = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                                                         vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    static constexpr vk::ImageUsageFlags COLOR_USAGE = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                                                       vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                                                       vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eStorage;
    static constexpr vk::ImageUsageFlags STORAGE_4U8_USAGE =
        vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    static constexpr vk::ImageUsageFlags STORAGE_USAGE =
        vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    image_formats_.at(as<usize>(ImageFormat::DEPTH)) = pickOptimalFormat(p_device_.o, DEPTH, DEPTH_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::TEXTURE_4U)) = pickOptimalFormat(p_device_.o, TEXTURE4C, TEXTURE_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::TEXTURE_4SRGB)) = pickOptimalFormat(p_device_.o, TEXTURE4C_SRGB, TEXTURE_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::TEXTURE_2U)) = pickOptimalFormat(p_device_.o, TEXTURE2C, TEXTURE_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::TEXTURE_1U)) = pickOptimalFormat(p_device_.o, TEXTURE1C, TEXTURE_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::GBUF_PARAMS)) = pickOptimalFormat(p_device_.o, GBUF_PARAMS, GBUF_PARAMS_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::NORMALS)) = pickOptimalFormat(p_device_.o, NORMALS, NORMALS_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::COLOR)) = pickOptimalFormat(p_device_.o, COLOR, COLOR_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::STORAGE_4U8)) = pickOptimalFormat(p_device_.o, STORAGE_4U8, STORAGE_4U8_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::STORAGE_F32)) = pickOptimalFormat(p_device_.o, STORAGE_F32, STORAGE_REQUIRED_FEATURES);
    image_formats_.at(as<usize>(ImageFormat::STORAGE_U32)) = pickOptimalFormat(p_device_.o, STORAGE_U32, STORAGE_REQUIRED_FEATURES);

    image_format_usages_.at(as<usize>(ImageFormat::DEPTH)) = DEPTH_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::TEXTURE_4U)) = TEXTURE_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::TEXTURE_4SRGB)) = TEXTURE_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::TEXTURE_2U)) = TEXTURE_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::TEXTURE_1U)) = TEXTURE_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::GBUF_PARAMS)) = GBUF_PARAMS_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::NORMALS)) = NORMALS_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::COLOR)) = COLOR_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::STORAGE_4U8)) = STORAGE_4U8_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::STORAGE_F32)) = STORAGE_USAGE;
    image_format_usages_.at(as<usize>(ImageFormat::STORAGE_U32)) = STORAGE_USAGE;

    for (usize i = 0; i < as<usize>(ImageFormat::COUNT); ++i) {
        auto format = image_formats_.at(i);
        if (format == vk::Format::eUndefined) {
            MLE_E("Failed to find a suitable format for {}", as<ImageFormat>(i));
        } else {
            MLE_D("Selected format for {}: {}, usage: {}", as<ImageFormat>(i), vk::to_string(format), vk::to_string(image_format_usages_.at(i)));
        }
    }
}

void VkCtx::initDevice() {
    MLE_D("Creating Vulkan device");

    pickPhysicalDevice();
    pickQueueIndices();
    pickImageFormats();

    std::vector<vk::DeviceQueueCreateInfo> queue_cis;
    std::array queue_priorities = {1.0F};

#ifdef MLE_IS_CLIENT
    auto& g_queue_ci = queue_cis.emplace_back();
    g_queue_ci.queueFamilyIndex = as<u32>(queue_data_.g_fam_idx);
    g_queue_ci.queueCount = 1;
    g_queue_ci.pQueuePriorities = queue_priorities.data();
#endif

    if (queue_data_.separate_compute) {
        auto& c_queue_ci = queue_cis.emplace_back();
        c_queue_ci.queueFamilyIndex = as<u32>(queue_data_.c_fam_idx);
        c_queue_ci.queueCount = 1;
        c_queue_ci.pQueuePriorities = queue_priorities.data();
    }

    if (queue_data_.dedicated_transfer) {
        auto& t_queue_ci = queue_cis.emplace_back();
        t_queue_ci.queueFamilyIndex = as<u32>(queue_data_.t_fam_idx);
        t_queue_ci.queueCount = 1;
        t_queue_ci.pQueuePriorities = queue_priorities.data();
    }

    // FIXME: this is trash, this must be app defined and needs to be checked for support
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features{};
    extended_dynamic_state_features.pNext = nullptr;

#ifdef MLE_IS_CLIENT
    extended_dynamic_state_features.extendedDynamicState = vk::True;
#endif

    vk::PhysicalDeviceVulkan12Features vulkan12_features{};
    vulkan12_features.pNext = &extended_dynamic_state_features;
    vulkan12_features.hostQueryReset = vk::True;
    vulkan12_features.bufferDeviceAddress = vk::True;

#ifdef MLE_IS_CLIENT
    vulkan12_features.descriptorIndexing = vk::True;
    vulkan12_features.runtimeDescriptorArray = vk::True;
    vulkan12_features.descriptorBindingVariableDescriptorCount = vk::True;
    vulkan12_features.descriptorBindingSampledImageUpdateAfterBind = vk::True;
    vulkan12_features.descriptorBindingUpdateUnusedWhilePending = vk::True;
    vulkan12_features.descriptorBindingPartiallyBound = vk::True;
#endif

    vk::PhysicalDeviceVulkan13Features vulkan13_features{};
    vulkan13_features.pNext = &vulkan12_features;
    vulkan13_features.synchronization2 = vk::True;

#ifdef MLE_IS_CLIENT
    vulkan13_features.dynamicRendering = vk::True;
#endif

    vk::PhysicalDeviceFeatures2 required_features{};
    required_features.pNext = &vulkan13_features;

#ifdef MLE_IS_CLIENT
    required_features.features.wideLines = vk::True;
#endif

    std::vector<const char*> required_extensions;
    required_extensions.push_back(vk::KHRPushDescriptorExtensionName);
#ifdef MLE_IS_CLIENT
    required_extensions.push_back(vk::KHRSwapchainExtensionName);
#endif

    vk::DeviceCreateInfo device_ci{};
    device_ci.pNext = &required_features;
    device_ci.setQueueCreateInfos(queue_cis);
    device_ci.setPEnabledExtensionNames(required_extensions);

    device_ = unwrap(p_device_.o.createDevice(device_ci));

#ifdef MLE_IS_CLIENT
    queue_data_.g_queue = device_.getQueue(as<u32>(queue_data_.g_fam_idx), 0);
#endif
    if (queue_data_.separate_compute) {
        queue_data_.c_queue = device_.getQueue(as<u32>(queue_data_.c_fam_idx), 0);
    }
    if (queue_data_.dedicated_transfer) {
        queue_data_.t_queue = device_.getQueue(as<u32>(queue_data_.t_fam_idx), 0);
    }
}

void VkCtx::initVMA() {
    MLE_D("Creating Vulkan Memory Allocator (VMA)");

    VmaAllocatorCreateInfo vma_ci{};
    vma_ci.physicalDevice = p_device_.o;
    vma_ci.device = device_;
    vma_ci.instance = vk_instance_;
    vma_ci.vulkanApiVersion = vk::ApiVersion13;
    vma_ci.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    check(as<vk::Result>(vmaCreateAllocator(&vma_ci, &vma_)));
}

void VkCtx::logDevice() {
    MLE_I("Selected physical device {}.", p_device_.properties.deviceName.data());
    MLE_I("Device type: {}", vk::to_string(p_device_.properties.deviceType));
    MLE_I("Driver version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.driverVersion), VK_VERSION_MINOR(p_device_.properties.driverVersion),
          VK_VERSION_PATCH(p_device_.properties.driverVersion));
    MLE_I("Vulkan API version: {}.{}.{}", VK_VERSION_MAJOR(p_device_.properties.apiVersion), VK_VERSION_MINOR(p_device_.properties.apiVersion),
          VK_VERSION_PATCH(p_device_.properties.apiVersion));
    MLE_I("Queue families: G:{} C:{} T:{}, separate_compute={}, dedicated_transfer={}", queue_data_.g_fam_idx, queue_data_.c_fam_idx, queue_data_.t_fam_idx,
          queue_data_.separate_compute, queue_data_.dedicated_transfer);
    MLE_I("Selected formats:");
    for (usize i = 0; i < as<usize>(ImageFormat::COUNT); ++i) {
        MLE_I("  {}: {}, usage: {}", as<ImageFormat>(i), vk::to_string(image_formats_.at(i)), vk::to_string(image_format_usages_.at(i)));
    }
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

[[nodiscard]] vk::DeviceSize VkCtx::getAlignmentForBufferUsage(vk::BufferUsageFlags flags) const {
    const auto& limits = p_device_.properties.limits;

    if (flags & vk::BufferUsageFlagBits::eUniformBuffer) {
        return limits.minUniformBufferOffsetAlignment;
    }

    if (flags & vk::BufferUsageFlagBits::eStorageBuffer) {
        return limits.minStorageBufferOffsetAlignment;
    }

    return 1;
}

std::string VkCtx::makePerfString() {
    const auto& props = p_device_.properties;
    const auto& heaps = p_device_.memory_properties.memoryHeaps;
    const u32 heap_cnt = p_device_.memory_properties.memoryHeapCount;

    auto ver_str = [](u32 v) { return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(v), VK_API_VERSION_MINOR(v), VK_API_VERSION_PATCH(v)); };

    std::vector<VmaBudget> budgets(heap_cnt);
    vmaGetHeapBudgets(vma_, budgets.data());

    u64 vram_used = 0, vram_budget = 0;
    u64 sys_used = 0, sys_budget = 0;
    for (u32 i = 0; i < heap_cnt; ++i) {
        const bool dev_local = as<bool>(heaps.at(i).flags & vk::MemoryHeapFlagBits::eDeviceLocal);
        if (dev_local) {
            vram_used += budgets[i].usage;
            vram_budget += budgets[i].budget;
        } else {
            sys_used += budgets[i].usage;
            sys_budget += budgets[i].budget;
        }
    }

    // VMA totals (allocs/blocks/bytes)
    VmaTotalStatistics ts{};
    vmaCalculateStatistics(vma_, &ts);
    const auto& s = ts.total.statistics;

    std::vector<std::string> lines;
    lines.emplace_back(fmt::format("{} ({})", std::string(props.deviceName), vk::to_string(props.deviceType)));
    lines.emplace_back(fmt::format("Driver {}", ver_str(props.driverVersion)));
    lines.emplace_back(fmt::format("Vulkan {}", ver_str(props.apiVersion)));
    lines.emplace_back(fmt::format("VMA allocs {} in {} blocks", s.allocationCount, s.blockCount));
    lines.emplace_back(fmt::format("Allocated {}", bitsToString(s.allocationBytes)));
    lines.emplace_back(fmt::format("VRAM {}/{}", bitsToString(vram_used), bitsToString(vram_budget)));
    if (sys_budget > 0) {
        lines.emplace_back(fmt::format("SysRAM {}/{}", bitsToString(sys_used), bitsToString(sys_budget)));
    }

    return fmt::format("{}", fmt::join(lines, "\n"));
}

Expected<ImageFormat> VkCtx::getFormat(const char* str) {
    auto id = entt::hashed_string{str};
    switch (id) {
        case entt::hashed_string{"color"}:
            return ImageFormat::COLOR;
        default:
            MLE_E("Unknown format string {}", str);
            return std::unexpected(Result::INVALID_ARGUMENT);
    }
};
}  // namespace mle
