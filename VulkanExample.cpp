// FOVE Vulkan Example
// This shows how to display content in a FOVE HMD via the FOVE SDK & Vulkan
#include "Model.h" // import levelModelVerts
#include "NativeUtil.h"
#include "Util.h"
#include <FoveAPI.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// shader data generated in `build/shaders`
#include <DemoScene.frag.spv.h>
#include <DemoScene.vert.spv.h>
#include <TextureCopy.frag.spv.h>
#include <TextureCopy.vert.spv.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_UNION_CONSTRUCTORS
#define VULKAN_HPP_NO_UNION_SETTERS
#ifdef __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>

#ifdef __linux__
#include "LinuxUtil.h"
#endif

// Define vk::defaultDispatchLoaderDynamic
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

// Use correct version of the Vulkan version macro
#ifdef VK_MAKE_API_VERSION
#define MAKE_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)
#else
#define MAKE_VERSION(major, minor, patch) VK_MAKE_VERSION(major, minor, patch)
#endif

// Use std namespace for convenience
using namespace std;

namespace
{

// Players height above the ground (in meters)
constexpr float playerHeight = 1.6f;

// Some configurations
constexpr auto appName = "FoveVulkanExample";
constexpr uint32_t N_MAX_FRAMES_IN_FLIGHT = 2U;

// Required extensions
constexpr auto requiredInstanceExtensions = array<const char* const, 3>{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_XLIB_SURFACE_EXTENSION_NAME, // FIXME Linux specific
	VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
};

constexpr auto requiredDeviceExtensions = array<const char* const, 3>{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,

	// VK_KHR_external_memory extension and its implementation on each OS
	// are necessary for submitting textures to Fove runtime.
	// On Linux, the implementation is VK_KHR_external_memory_fd.
	VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
	VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME // FIXME Linux specific
};

// Debug settings
constexpr auto disableValidationLayers = false;
constexpr auto disableDebugUtils = false;
constexpr auto validationLayerName = "VK_LAYER_KHRONOS_validation";
PFN_vkCreateDebugUtilsMessengerEXT g_vkCreateDebugUtilsMessengerEXT{nullptr};
PFN_vkDestroyDebugUtilsMessengerEXT g_vkDestroyDebugUtilsMessengerEXT{nullptr};
PFN_vkSubmitDebugUtilsMessageEXT g_vkSubmitDebugUtilsMessageEXT{nullptr};

#ifndef FOVE_CPP_SAMPLE_BUILD_IN_TREE
extern "C"
{

	VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
	{
		return g_vkCreateDebugUtilsMessengerEXT(instance, createInfo, pAllocator, pMessenger);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		return g_vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
	}

	VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
	{
		return g_vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData);
	}

} // extern "C"
#endif

struct QueueFamily
{
	uint32_t index;
};

// Make it match the alignment of the data from Model.h (float levelModelVerts[]) (pos: 4 floats, color: 3 floats).
// But note that the pos.w is 0.0F in the data, so we need to set it to 1.0F in the shader.
struct RenderTextureVertex
{
	float pos[4];
	float color[3];

	// Type of the associated index buffer.
	// Should be large enough to address all verticies in the vetex buffer.
	using IndexType = uint16_t;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RenderTextureVertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingDescription;
	}

	static array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32A32Sfloat;
		attributeDescriptions[0].offset = offsetof(RenderTextureVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(RenderTextureVertex, color);

		return attributeDescriptions;
	}
};
static_assert(sizeof(RenderTextureVertex) == 7 * sizeof(float));

struct RenderTextureUbo
{
	Fove::Matrix44 mvp{};
	// For highlighting the gazed object in the scene.
	// A valid object has non-negative integral value (stored in a float for simplicity)
	float selection{-1.0F};
};
struct RenderTextureUboLR
{
	RenderTextureUbo uboL;
	RenderTextureUbo uboR;
};
static_assert(sizeof(RenderTextureUbo) == 17 * sizeof(float));
static_assert(sizeof(RenderTextureUboLR) == 2 * sizeof(RenderTextureUbo));

struct SwapchainVertex
{
	float pos[2];
	float uv[2];

	using IndexType = uint16_t;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(SwapchainVertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingDescription;
	}

	static array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[0].offset = offsetof(SwapchainVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(SwapchainVertex, uv);

		return attributeDescriptions;
	}
};
static_assert(sizeof(SwapchainVertex) == 4 * sizeof(float));

struct SwapchainUbo
{
	float dummy;
};

template <typename T>
class Span
{
public:
	Span(Span&&) = default;
	Span(const Span&) = default;

	template <size_t N>
	explicit Span(float (&data)[N])
		: m_data{reinterpret_cast<T*>(data)}, m_size{(N * sizeof(float)) / sizeof(T)}
	{
	}

	// This is a weired api..
	template <size_t N>
	explicit Span(const float (&data)[N])
		: m_data{reinterpret_cast<const T*>(data)}, m_size{(N * sizeof(float)) / sizeof(T)}
	{
	}

	template <size_t N>
	explicit Span(array<T, N>& data)
		: m_data{reinterpret_cast<T*>(data)}, m_size{N / sizeof(T)}
	{
	}

	template <size_t N>
	explicit Span(const array<T, N>& data)
		: m_data{reinterpret_cast<const T*>(data)}, m_size{N / sizeof(T)}
	{
	}

	explicit Span(vector<T>& vs)
		: m_data{vs.data()}, m_size{vs.size()}
	{
	}

	explicit Span(const vector<remove_cv_t<T>>& vs)
		: m_data{vs.data()}, m_size{vs.size()}
	{
	}

	~Span() = default;

	const T* data() const { return m_data; }
	T* data() { return m_data; }
	size_t size() const { return m_size; }

private:
	T* m_data;
	size_t m_size;
};

// This is not used, but has the same structure as levelModelVerts from Model.h
const vector<RenderTextureVertex> g_vertices = {
	// Each vertex has 7 attributes (x,y,z,selectionId; r,g,b) organized as vec4+vec3
	{{1.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
	{{-1.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}},
	{{0.0F, -1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}},
};

// For showing render texture on host screen
const vector<SwapchainVertex> g_vertices2 = {
	{{-1.0F, -1.0F}, {0.0F, 0.0F}},
	{{1.0F, -1.0F}, {1.0F, 0.0F}},
	{{1.0F, 1.0F}, {1.0F, 1.0F}},
	{{-1.0F, 1.0F}, {0.0F, 1.0F}}};
const vector<SwapchainVertex::IndexType> g_indices2 = {0, 1, 2, 2, 3, 0};

// Wait for all device resources to be destroyed before destroying the device
struct VulkanWaitDeviceIdle
{
	vk::Device device;

	~VulkanWaitDeviceIdle()
	{
		device.waitIdle();
	}
};

// vk::Device::waitIdle() does not automatically wait for fences the device has created
struct VulkanWaitAllFences
{
	vk::Device device;
	vector<vk::Fence> fences;

	~VulkanWaitAllFences()
	{
		const bool waitAll{true};
		if (const vk::Result res = device.waitForFences(fences, waitAll, chrono::duration_cast<chrono::nanoseconds>(chrono::milliseconds(2500)).count());
			res != vk::Result::eSuccess)
		{
			cerr << "Failed to wait for a GPU fence: " << to_string(res) << '\n';
		}
	}
};

class VulkanResources
{
public:
	void createInstance();
	void setupDebugMessenger();
	void createXlibSurface(XDisplay*, XWindow); // TODO Linux specific
	void pickPhysicalDevice();
	void pickQueue();
	void createLogicalDevice();

	// Render texture to be submitted to Fove runtime
	void createRenderTextureImages(const uint32_t nImages, const uint32_t width, const uint32_t height);
	void createRenderTextureDeviceMemories();
	void createRenderTextureImageViews();
	void createRenderTextureRenderPass();
	void createRenderTextureDescriptorSetLayout();
	void createRenderTextureGraphicsPipeline();
	void createRenderTextureFramebuffers();
	void createRenderTextureVertexBuffer(const Span<const RenderTextureVertex>);
	void createRenderTextureIndexBuffer(const Span<const RenderTextureVertex::IndexType>); // not used
	void createRenderTextureUniformBuffers();
	void createRenderTextureDescriptorPool();
	void createRenderTextureDescriptorSets();

	// Swapchains for the host display.
	// This is not necessary for rendering to the headset through Fove runtime,
	// but this example renders the same content on the host display as well.
	void createSwapchain(const uint32_t width, const uint32_t height);
	void createSwapchainImages();
	void createSwapchainImageViews(); // vulkan image views for the swapchain
	void createSwapchainRenderPass();
	void createSwapchainDescriptorSetLayout();
	void createSwapchainGraphicsPipeline();
	void createSwapchainFramebuffers();
	void createSwapchainVertexBuffer(const Span<const SwapchainVertex>);
	void createSwapchainIndexBuffer(const Span<const SwapchainVertex::IndexType>);
	void createSwapchainTextureSampler();
	void createSwapchainUniformBuffers();
	void createSwapchainDescriptorPool();
	void createSwapchainDescriptorSets();

	void createCommandPool();
	void createCommandBuffers(const uint32_t nImages);
	void createSyncObjects(const uint32_t nImages, const uint32_t nMaxFramesInFlight);

	// Helpers
	void cleanupSwapchain();
	void recreateSwapchain(NativeWindow&);

	// Main rendering logic
	// First renders the scene to a render texture and shows the result to the host screen quad for monitoring.
	// The same texture is then submitted to Fove runtime by Fove::Compositor::submit() API.
	void recordCommandBuffers(const Span<const RenderTextureVertex> sceneVerts, const Span<const SwapchainVertex::IndexType> quadInds);

	// App interface
	uint32_t drawFrame(NativeWindow&, const RenderTextureUboLR&);

private:
	// Each image uses two descriptor sets, one for left, another for right
	void updateRenderTextureUniformBuffer(const uint32_t descriptorIndex, const RenderTextureUbo&);
	void updateSwapchainUniformBuffer();

private:
	friend class VulkanExample;
	bool m_enableValidationLayers{false};
	bool m_enableDebugUtils{false};
	vk::UniqueInstance m_instance{};
	vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger{};
	vk::UniqueSurfaceKHR m_surface{};

	vk::PhysicalDevice m_physicalDevice{};
	QueueFamily m_queueFamily{};
	vk::UniqueDevice m_device{};
	VulkanWaitDeviceIdle m_deviceWaitIdle{m_device.get()};
	vk::Queue m_queue{}; // we share graphics/presentation queues

	////////////////////////////////
	// Render to texture for submission to Fove runtime
	vk::Format m_renderTextureImageFormat{};
	vk::Extent2D m_renderTextureExtent{};
	vector<vk::UniqueDeviceMemory> m_renderTextureDeviceMemories{};
	vector<vk::UniqueImage> m_renderTextureImages{};
	vector<vk::UniqueImageView> m_renderTextureImageViews{};
	vector<vk::UniqueFramebuffer> m_renderTextureFramebuffers{};

	vk::UniqueRenderPass m_renderTextureRenderPass{};
	vk::UniqueDescriptorSetLayout m_renderTextureDescriptorSetLayout{};
	vk::UniquePipelineLayout m_renderTexturePipelineLayout{};
	vk::UniquePipeline m_renderTextureGraphicsPipeline{};

	vk::UniqueBuffer m_renderTextureVertexBuffer{};
	vk::UniqueDeviceMemory m_renderTextureVertexBufferMemory{};
	vk::UniqueBuffer m_renderTextureIndexBuffer{};
	vk::UniqueDeviceMemory m_renderTextureIndexBufferMemory{};

	vector<vk::UniqueBuffer> m_renderTextureUniformBuffers{};
	vector<vk::UniqueDeviceMemory> m_renderTextureUniformBufferMemories{};
	vk::UniqueDescriptorPool m_renderTextureDescriptorPool{};
	vector<vk::UniqueDescriptorSet> m_renderTextureDescriptorSets{};

	////////////////////////////////
	// Render to host screen
	vk::Format m_swapchainImageFormat{};
	vk::Extent2D m_swapchainExtent{};
	vk::UniqueSwapchainKHR m_swapchain{};
	vector<vk::Image> m_swapchainImages{};
	vector<vk::UniqueImageView> m_swapchainImageViews{};
	vector<vk::UniqueFramebuffer> m_swapchainFramebuffers{};

	vk::UniqueRenderPass m_swapchainRenderPass{};
	vk::UniqueDescriptorSetLayout m_swapchainDescriptorSetLayout{};
	vk::UniquePipelineLayout m_swapchainPipelineLayout{};
	vk::UniquePipeline m_swapchainGraphicsPipeline{};

	vk::UniqueBuffer m_swapchainVertexBuffer{};
	vk::UniqueDeviceMemory m_swapchainVertexBufferMemory{};
	vk::UniqueBuffer m_swapchainIndexBuffer{};
	vk::UniqueDeviceMemory m_swapchainIndexBufferMemory{};

	// We do not use uniform buffers for this example, but we leave it here as a reference
	vector<vk::UniqueBuffer> m_swapchainUniformBuffers{};
	vector<vk::UniqueDeviceMemory> m_swapchainUniformBufferMemories{};
	vk::UniqueSampler m_swapchainTextureSampler{};
	vk::UniqueDescriptorPool m_swapchainDescriptorPool{};
	vector<vk::UniqueDescriptorSet> m_swapchainDescriptorSets{};

	atomic_bool m_swapchainFramebufferResized{false};

	////////////////////////////////
	vk::UniqueCommandPool m_commandPool{};
	vector<vk::UniqueCommandBuffer> m_commandBuffers{};

	vector<vk::UniqueSemaphore> m_imageAvailableSemaphores{};
	vector<vk::UniqueSemaphore> m_renderFinishedSemaphores{};
	vector<vk::UniqueFence> m_inFlightFences{};
	vector<vk::Fence> m_imageInUseFences{};
	VulkanWaitAllFences m_waitAllFences{};
	size_t m_currentFrame = 0;
};

////////////////////////////////
// Some helper functions.
//
// NOTE Most of these are not meant to be generic enough to cover common use cases
// and are unlikely to be useful outside the context of this example.
void loadDebugMessengers(const vk::Instance instance)
{
	if (g_vkCreateDebugUtilsMessengerEXT == nullptr)
		g_vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
	if (g_vkDestroyDebugUtilsMessengerEXT == nullptr)
		g_vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
	if (g_vkSubmitDebugUtilsMessageEXT == nullptr)
		g_vkSubmitDebugUtilsMessageEXT = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(instance.getProcAddr("vkSubmitDebugUtilsMessengerEXT"));
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void*)
{
	const auto message = pCallbackData->pMessage;

	const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags = [severity] {
		vk::DebugUtilsMessageSeverityFlagsEXT flags;
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			flags |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			flags |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			flags |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			flags |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		return flags;
	}();

	const vk::DebugUtilsMessageTypeFlagsEXT typeFlags = [messageTypes] {
		vk::DebugUtilsMessageTypeFlagsEXT flags;
		if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			flags |= vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral;
		if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			flags |= vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			flags |= vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		return flags;
	}();

	if (severityFlags & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
	{
		cerr << "Validation Layer: " << to_string(severityFlags) << ": " << to_string(typeFlags) << ": " << message << '\n';
	}
	else if (severityFlags & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		cerr << "Validation Layer: " << to_string(severityFlags) << ": " << to_string(typeFlags) << ": " << message << '\n';
	}
	else if (severityFlags & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
	{
		cerr << "Validation Layer: " << to_string(severityFlags) << ": " << to_string(typeFlags) << ": " << message << '\n';
	}
	else
	{
		cerr << "Validation Layer: " << to_string(severityFlags) << ": " << to_string(typeFlags) << ": " << message << '\n';
	}

	return VK_FALSE;
}

bool checkValidationLayerSupport()
{
	const vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
	for (const auto& layerProperties : availableLayers)
		if (strcmp(validationLayerName, layerProperties.layerName) == 0)
			return true;
	return false;
}

bool checkDebugUtilsSupport()
{
	const vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
	for (const auto& extensionProperties : availableExtensions)
		if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProperties.extensionName))
			return true;
	return false;
}

// Helper function for when the vulkan c++ api might return a ResultValue or a UniqueHandle, depending on version/settings
template <typename T>
auto checkAndGetResult(vk::ResultValue<T> res)
{
	static_assert(std::is_move_constructible_v<T>);
	if (res.result != vk::Result::eSuccess)
	{
		throw "vulkan call failed"; // Should do better in production code
	}
	return std::move(res.value);
}
template <typename Type, typename Deleter>
auto checkAndGetResult(vk::UniqueHandle<Type, Deleter> res)
{
	static_assert(std::is_move_constructible_v<vk::UniqueHandle<Type, Deleter>>);
	return res;
}

struct BufferAndMemory
{
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory deviceMemory;
};

uint32_t findMemoryTypeIndex(const vk::MemoryRequirements reqs, const vk::PhysicalDeviceMemoryProperties props, const vk::MemoryPropertyFlags flags)
{
	for (auto i = 0u; i < props.memoryTypeCount; ++i)
	{
		if ((reqs.memoryTypeBits & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
			return i;
	}
	throw "Failed to find suitable memory type";
}

uint32_t findMemoryType(
	const vk::PhysicalDevice physicalDevice,
	const vk::MemoryRequirements reqs,
	const vk::MemoryPropertyFlags flags)
{
	vk::PhysicalDeviceMemoryProperties props;
	physicalDevice.getMemoryProperties(&props);
	return findMemoryTypeIndex(reqs, props, flags);
}

// In real applications, we might want to use different queue families for graphics/presentation,
// but for simplicity we settle on a queue that has both capabilities.
optional<QueueFamily> findQueueFamilies(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
{
	vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			const vk::Bool32 presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);
			if (presentSupport)
			{
				return QueueFamily{i};
			}
		}
		++i;
	}
	return {};
}

vk::UniqueImage createTextureImage(const vk::Device device,
								   const vk::Extent2D extent,
								   const uint32_t numLayers,
								   const vk::Format format,
								   const vk::ImageTiling tiling,
								   const vk::ImageUsageFlags usage)
{
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent = vk::Extent3D{extent.width, extent.height, static_cast<uint32_t>(1)};
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = numLayers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive; // one queue family at a time
	imageInfo.samples = vk::SampleCountFlagBits::e1;

	vk::ExternalMemoryImageCreateInfo externalInfo{};
#ifdef __linux__
	externalInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
	imageInfo.pNext = &externalInfo;
#else
#error "Texture exporting not implemented on this platform"
#endif
	return device.createImageUnique(imageInfo);
}

vk::UniqueImageView createTextureImageView(const vk::Device device, const vk::Image image, const uint32_t numLayers, const vk::ImageViewType viewType, const vk::Format format)
{
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = numLayers;
	return device.createImageViewUnique(viewInfo);
}

vk::UniqueDeviceMemory createTextureDeviceMemory(const vk::Device device,
												 const vk::Image image,
												 const vk::MemoryRequirements memReqs,
												 const vk::PhysicalDeviceMemoryProperties devMemProps,
												 const vk::MemoryPropertyFlags memFlags)
{
	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex(memReqs, devMemProps, memFlags);

// Prepare for import/export
#ifdef __linux__
	vk::MemoryDedicatedAllocateInfo dedicatedAllocInfo{};
	dedicatedAllocInfo.image = image;
	vk::ExportMemoryAllocateInfo exportAllocInfo{};
	exportAllocInfo.pNext = &dedicatedAllocInfo;
	exportAllocInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
	allocInfo.pNext = &exportAllocInfo;
#else
#error "texture import/export not implemented on this platform"
#endif
	return device.allocateMemoryUnique(allocInfo);
}

vk::UniqueSampler createTextureSampler(const vk::Device device)
{
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE; // use normalized coord
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	return device.createSamplerUnique(samplerInfo);
}

BufferAndMemory createBufferAndMemory(
	const vk::PhysicalDevice physicalDevice,
	const vk::Device device,
	const vk::DeviceSize size,
	const vk::BufferUsageFlags usage,
	const vk::MemoryPropertyFlags properties,
	const uint32_t queueFamilyIndex)
{
	const vk::BufferCreateInfo bufferInfo = [size, usage, &queueFamilyIndex] {
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.queueFamilyIndexCount = 1U;
		bufferInfo.pQueueFamilyIndices = &queueFamilyIndex;
		return bufferInfo;
	}();

	auto buffer = device.createBufferUnique(bufferInfo);

	const vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer.get());

	const vk::MemoryAllocateInfo allocInfo = [physicalDevice, properties, &memRequirements] {
		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements, properties);
		return allocInfo;
	}();
	auto bufferMemory = device.allocateMemoryUnique(allocInfo);

	const vk::DeviceSize offset{0};
	device.bindBufferMemory(buffer.get(), bufferMemory.get(), offset);
	return BufferAndMemory{std::move(buffer), std::move(bufferMemory)};
}

struct BuffersAndMemories
{
	vector<vk::UniqueBuffer> buffers;
	vector<vk::UniqueDeviceMemory> deviceMemories;
};

BuffersAndMemories createBuffersAndMemories(
	const vk::PhysicalDevice physicalDevice,
	const vk::Device device,
	const size_t swapchainLength,
	const vk::DeviceSize bufferSize,
	const vk::BufferUsageFlags usage,
	const vk::MemoryPropertyFlags properties,
	uint32_t queueFamilyIndex)
{
	vector<vk::UniqueBuffer> buffers;
	buffers.reserve(swapchainLength);

	vector<vk::UniqueDeviceMemory> deviceMemories;
	deviceMemories.reserve(swapchainLength);

	for (size_t i = 0; i < swapchainLength; ++i)
	{
		auto res = createBufferAndMemory(physicalDevice, device, bufferSize, usage, properties, queueFamilyIndex);
		buffers.emplace_back(std::move(res.buffer));
		deviceMemories.emplace_back(std::move(res.deviceMemory));
	}
	return BuffersAndMemories{std::move(buffers), std::move(deviceMemories)};
}

void copyBuffer(
	const vk::Device device,
	const vk::CommandPool commandPool,
	const vk::Queue graphicsQueue,
	const vk::Buffer srcBuffer,
	const vk::Buffer dstBuffer,
	const vk::DeviceSize size)
{
	const vk::CommandBufferAllocateInfo allocInfo = [commandPool] {
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;
		return allocInfo;
	}();

	const vector<vk::UniqueCommandBuffer> commandBuffers = device.allocateCommandBuffersUnique(allocInfo);
	const vk::CommandBuffer commandBuffer = commandBuffers[0].get();

	const vk::BufferCopy copyRegion = [commandBuffer, srcBuffer, dstBuffer, size] {
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		return copyRegion;
	}();

	const vk::CommandBufferBeginInfo beginInfo = [] {
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		return beginInfo;
	}();

	// Record command buffer
	commandBuffer.begin(beginInfo);
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
	commandBuffer.end();

	const vk::SubmitInfo submitInfo = [&commandBuffer] {
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		return submitInfo;
	}();

	const auto res = graphicsQueue.submit(1, &submitInfo, nullptr); // no fence
	if (res == vk::Result::eErrorOutOfDateKHR)
	{
		cerr << "Surface out of data (maybe resized)\n";
	}
	if (res != vk::Result::eSuccess)
	{
		throw "Failed to submit graphics queue";
	}
	graphicsQueue.waitIdle();
}

vk::UniqueRenderPass createSimpleRenderPass(
	const vk::Device device,
	const vk::Format imageFormat,
	const vk::ImageLayout finalLayout)
{
	const vk::AttachmentDescription colorAttachment = [imageFormat, finalLayout] {
		vk::AttachmentDescription colorAttachment;
		colorAttachment.format = imageFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = finalLayout;
		return colorAttachment;
	}();

	const vk::AttachmentReference colorAttachmentRef = [] {
		vk::AttachmentReference colorAttachmentRef;
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
		return colorAttachmentRef;
	}();

	const vk::SubpassDescription subpass = [&colorAttachmentRef] {
		vk::SubpassDescription subpass;
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		return subpass;
	}();

	const array<vk::SubpassDependency, 2> dependencies = [] {
		vk::SubpassDependency dependencyIn;
		dependencyIn.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencyIn.dstSubpass = {};
		dependencyIn.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		dependencyIn.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencyIn.srcAccessMask = vk::AccessFlagBits::eShaderRead;
		dependencyIn.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependencyIn.dependencyFlags = vk::DependencyFlagBits::eByRegion;

		vk::SubpassDependency dependencyOut;
		dependencyOut.srcSubpass = {};
		dependencyOut.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencyOut.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencyOut.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		dependencyOut.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependencyOut.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		dependencyOut.dependencyFlags = vk::DependencyFlagBits::eByRegion;

		return array{dependencyIn, dependencyOut};
	}();

	const vk::RenderPassCreateInfo renderPassInfo = [&colorAttachment, &subpass, &dependencies] {
		vk::RenderPassCreateInfo renderPassInfo;
		renderPassInfo.flags = {};
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();
		return renderPassInfo;
	}();

	return device.createRenderPassUnique(renderPassInfo);
}

vk::UniqueDescriptorSetLayout createDescriptorSetLayout(const vk::Device device, const uint32_t uboDescriptorCount,
														const uint32_t samplerDescriptorCount)
{
	vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.reserve(uboDescriptorCount + samplerDescriptorCount);

	// create layout for uniorm buffer objects and samplers.
	// Assume that all ubos have earlier binding number than samplers
	for (auto i = 0U; i < uboDescriptorCount; ++i)
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding{};
		{
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
			uboLayoutBinding.descriptorCount = uboDescriptorCount;
			uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			uboLayoutBinding.pImmutableSamplers = nullptr;
		}
		bindings.emplace_back(std::move(uboLayoutBinding));
	}

	// for texture sampler
	for (auto i = 0U; i < samplerDescriptorCount; ++i)
	{
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = uboDescriptorCount + i;
		samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerLayoutBinding.descriptorCount = samplerDescriptorCount;
		samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		bindings.emplace_back(std::move(samplerLayoutBinding));
	}

	vk::DescriptorSetLayoutCreateInfo createInfo{};
	createInfo.pBindings = bindings.data();
	createInfo.bindingCount = bindings.size();

	return device.createDescriptorSetLayoutUnique(createInfo);
}

vk::UniquePipelineLayout createSimpleGraphicsPipelineLayout(const vk::Device device,
															const vector<vk::DescriptorSetLayout>& setLayouts)
{
	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo = [&setLayouts] {
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = setLayouts.size();
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		return pipelineLayoutInfo;
	}();
	return device.createPipelineLayoutUnique(pipelineLayoutInfo);
}

vk::UniqueShaderModule createShaderModule(const vk::Device device, const vector<unsigned char>& code)
{
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.flags = {};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::UniqueShaderModule shaderModule = device.createShaderModuleUnique(createInfo);
	return shaderModule;
}

vk::UniquePipeline createSimpleGraphicsPipeline(const vk::Device device,
												const vector<unsigned char>& vertShaderCode,
												const vector<unsigned char>& fragShaderCode,
												const vk::Extent2D extent,
												const vk::PipelineLayout pipelineLayout,
												const vk::RenderPass renderPass,
												const vk::PipelineVertexInputStateCreateInfo& vertexInputState)
{
	const vk::UniqueShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
	const vk::UniqueShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

	const vk::PipelineShaderStageCreateInfo vertShaderStageInfo = [&vertShaderModule] {
		vk::PipelineShaderStageCreateInfo createInfo;
		createInfo.stage = vk::ShaderStageFlagBits::eVertex;
		createInfo.pName = "main";
		createInfo.module = vertShaderModule.get();
		return createInfo;
	}();

	const vk::PipelineShaderStageCreateInfo fragShaderStageInfo = [&fragShaderModule] {
		vk::PipelineShaderStageCreateInfo createInfo;
		createInfo.stage = vk::ShaderStageFlagBits::eFragment;
		createInfo.pName = "main";
		createInfo.module = fragShaderModule.get();
		return createInfo;
	}();
	const array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = [] {
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
		inputAssembly.flags = {};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		return inputAssembly;
	}();

	const vk::PipelineViewportStateCreateInfo viewportState = [] {
		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.flags = {};
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;
		return viewportState;
	}();

	const array<vk::DynamicState, 2> dynamicStateEnables{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	const vk::PipelineDynamicStateCreateInfo dynamicState = [&dynamicStateEnables] {
		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		return dynamicState;
	}();

	const vk::PipelineRasterizationStateCreateInfo rasterizationState = [] {
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		rasterizer.flags = {};
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.cullMode = vk::CullModeFlagBits::eBack;
		rasterizer.polygonMode = vk::PolygonMode::eFill;
		rasterizer.frontFace = vk::FrontFace::eClockwise;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;
		rasterizer.lineWidth = 1.0f;
		return rasterizer;
	}();

	const vk::PipelineMultisampleStateCreateInfo multisampleState = [] {
		vk::PipelineMultisampleStateCreateInfo multisampleState;
		multisampleState.flags = {};
		multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisampleState.sampleShadingEnable = VK_FALSE;
		return multisampleState;
	}();

	const vk::PipelineColorBlendAttachmentState colorBlendAttachment = [] {
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		return colorBlendAttachment;
	}();

	const vk::PipelineColorBlendStateCreateInfo colorBlendState = [&colorBlendAttachment] {
		vk::PipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.flags = {};
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = vk::LogicOp::eCopy;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
		return colorBlendState;
	}();

	const vk::GraphicsPipelineCreateInfo pipelineInfo = [&shaderStages, &vertexInputState, &inputAssemblyState, &viewportState, &rasterizationState, &multisampleState, &colorBlendState, &dynamicState, &pipelineLayout, &renderPass] {
		vk::GraphicsPipelineCreateInfo pipelineInfo;
		pipelineInfo.flags = {};
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pTessellationState = {};
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pDepthStencilState = {};
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0u;
		pipelineInfo.basePipelineHandle = nullptr;
		pipelineInfo.basePipelineIndex = {};
		return pipelineInfo;
	}();

	return checkAndGetResult(device.createGraphicsPipelineUnique({}, pipelineInfo));
}

vector<vk::UniqueFramebuffer> createFramebuffers(const vk::Device device,
												 const vector<vk::UniqueImageView>& imageViews,
												 const vk::RenderPass renderPass,
												 const vk::Extent2D extent)
{
	vector<vk::UniqueFramebuffer> framebuffers{};
	framebuffers.reserve(imageViews.size());
	for (auto const& view : imageViews)
	{
		const array<vk::ImageView, 1> views = {view.get()};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.flags = {};
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = views.size();
		framebufferInfo.pAttachments = views.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		framebuffers.emplace_back(device.createFramebufferUnique(framebufferInfo));
	}
	return framebuffers;
}

template <typename Vert, typename = enable_if_t<is_trivially_copyable_v<Vert>>>
BufferAndMemory createVertexBuffer(
	const vk::PhysicalDevice physicalDevice,
	const vk::Device device,
	const vk::CommandPool commandPool,
	const uint32_t queueFamilyIndex,
	const vk::Queue queue,
	const Span<const Vert> verts)
{
	const vk::DeviceSize bufferSize = sizeof(Vert) * verts.size();
	auto staging = createBufferAndMemory(
		physicalDevice,
		device,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		queueFamilyIndex);

	void* data;
	const vk::DeviceSize offset{0};
	const auto res = device.mapMemory(
		staging.deviceMemory.get(), offset, bufferSize, vk::MemoryMapFlagBits{}, &data);
	if (res != vk::Result::eSuccess)
	{
		throw "Failed to map vertex buffer";
	}
	memcpy(data, verts.data(), static_cast<size_t>(bufferSize));
	device.unmapMemory(staging.deviceMemory.get());

	auto bufAndMem = createBufferAndMemory(
		physicalDevice,
		device,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		queueFamilyIndex);

	copyBuffer(
		device,
		commandPool,
		queue,
		staging.buffer.get(),
		bufAndMem.buffer.get(),
		bufferSize);
	device.destroyBuffer(staging.buffer.release());
	device.freeMemory(staging.deviceMemory.release());

	return bufAndMem;
}

template <typename Index, typename = enable_if_t<is_arithmetic_v<Index>>>
BufferAndMemory createIndexBuffer(
	const vk::PhysicalDevice physicalDevice,
	const vk::Device device,
	const vk::CommandPool commandPool,
	const uint32_t queueFamilyIndex,
	const vk::Queue queue,
	const Span<const Index> inds)
{
	const vk::DeviceSize bufferSize = sizeof(Index) * inds.size();
	auto staging = createBufferAndMemory(
		physicalDevice,
		device,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		queueFamilyIndex);

	void* data;
	const vk::DeviceSize offset{0};
	const auto res = device.mapMemory(
		staging.deviceMemory.get(), offset, bufferSize, vk::MemoryMapFlagBits{}, &data);
	if (res != vk::Result::eSuccess)
	{
		throw "Failed to map vertex buffer";
	}
	memcpy(data, inds.data(), static_cast<size_t>(bufferSize));
	device.unmapMemory(staging.deviceMemory.get());

	auto bufAndMem = createBufferAndMemory(
		physicalDevice,
		device,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		queueFamilyIndex);

	copyBuffer(
		device,
		commandPool,
		queue,
		staging.buffer.get(),
		bufAndMem.buffer.get(),
		bufferSize);

	device.destroyBuffer(staging.buffer.release());
	device.freeMemory(staging.deviceMemory.release());
	return bufAndMem;
}

struct SwapchainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	vector<vk::SurfaceFormatKHR> formats;
	vector<vk::PresentModeKHR> presentModes;
};

SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
{
	SwapchainSupportDetails details;
	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const vector<vk::SurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		cout << "available format: " << to_string(availableFormat.format) << ", " << to_string(availableFormat.colorSpace) << '\n';
	}
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			cout << "Chosen format: " << to_string(availableFormat.format) << ", " << to_string(availableFormat.colorSpace) << '\n';
			return availableFormat;
		}
	}

	cout << "defaulting to: " << to_string(availableFormats[0].format) << ", " << to_string(availableFormats[0].colorSpace) << '\n';
	cout << flush;
	return availableFormats[0];
}

vk::PresentModeKHR chooseSwapchainPresentMode(const vector<vk::PresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eFifoRelaxed)
		{
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const uint32_t width, const uint32_t height)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		vk::Extent2D actualExtent;
		actualExtent.width = clamp(
			width,
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);
		actualExtent.height = clamp(
			height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

////////////////////////////////
void VulkanResources::createInstance()
{
	cout << "Creating Vulkan instance\n"
		 << flush;
	vk::ApplicationInfo appInfo{};
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "NoEngine";
	appInfo.engineVersion = MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	m_enableDebugUtils = !disableDebugUtils && checkDebugUtilsSupport();
	m_enableValidationLayers = !disableValidationLayers && checkValidationLayerSupport();

	vector<const char*> enabledExtensions{requiredInstanceExtensions.begin(), requiredInstanceExtensions.end()};
	if (m_enableDebugUtils)
	{
		enabledExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	const auto enabledLayers = m_enableValidationLayers
								   ? vector<const char*>{validationLayerName}
								   : vector<const char*>{};
	const vk::InstanceCreateInfo createInfo = [&appInfo, &enabledExtensions, &enabledLayers] {
		vk::InstanceCreateInfo createInfo;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.enabledExtensionCount = enabledExtensions.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();
		createInfo.enabledLayerCount = enabledLayers.size();
		return createInfo;
	}();
	m_instance = vk::createInstanceUnique(createInfo);

	// get all the other function pointers
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
}

void VulkanResources::setupDebugMessenger()
{
	cout << "Setting up Vulkan debug utils\n"
		 << flush;

	const vk::Instance instance = m_instance.get();
	loadDebugMessengers(instance);
	vk::DebugUtilsMessengerCreateInfoEXT createInfo = [] {
		const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
		const vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation};

		vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.messageSeverity = severityFlags;
		createInfo.messageType = messageTypeFlags;
		createInfo.pfnUserCallback = debugUtilsMessengerCallback;
		return createInfo;
	}();
	m_debugUtilsMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createInfo);
}

void VulkanResources::createXlibSurface(XDisplay* xDisplay, XWindow window)
{
	vk::XlibSurfaceCreateInfoKHR createInfo{};
	createInfo.dpy = xDisplay;
	createInfo.window = window;
	m_surface = m_instance.get().createXlibSurfaceKHRUnique(createInfo);
}

void VulkanResources::pickPhysicalDevice()
{
	const vector<vk::PhysicalDevice> physicalDevices = m_instance->enumeratePhysicalDevices();
	if (physicalDevices.empty())
	{
		throw "Failed to find a suitable GPU!";
	}
	// When there are multiple physical devices available, one should inspect the list
	// and choose the same physical device used by Fove runtime.
	m_physicalDevice = physicalDevices[0];
}

void VulkanResources::pickQueue()
{
	const optional<QueueFamily> queueFamily_ = findQueueFamilies(m_physicalDevice, m_surface.get());
	if (!queueFamily_)
	{
		throw "Cannot find queue family with graphics and presentation capabilities";
	}
	m_queueFamily = queueFamily_.value();
	cout << "Queue family index: " << m_queueFamily.index << '\n'
		 << flush;
}

void VulkanResources::createLogicalDevice()
{
	vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
	{
		const auto queuePriorities = array<float, 1>{1.0f};
		for (const auto queueFamily : array<QueueFamily, 1>{m_queueFamily})
		{
			vk::DeviceQueueCreateInfo queueCreateInfo = [&queuePriorities, queueFamily] {
				vk::DeviceQueueCreateInfo queueCreateInfo;
				queueCreateInfo.queueCount = queuePriorities.size();
				queueCreateInfo.pQueuePriorities = queuePriorities.data();
				queueCreateInfo.queueFamilyIndex = queueFamily.index;
				return queueCreateInfo;
			}();
			queueCreateInfos.emplace_back(queueCreateInfo);
		}
	}

	const vk::PhysicalDeviceFeatures deviceFeatures{};
	const auto validationLayers = m_enableValidationLayers ? vector<const char*>{validationLayerName}
														   : vector<const char*>{};
	const vk::DeviceCreateInfo createInfo = [&deviceFeatures, &queueCreateInfos, &validationLayers] {
		vk::DeviceCreateInfo createInfo;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = queueCreateInfos.size();
		createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
		createInfo.enabledExtensionCount = requiredDeviceExtensions.size();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.enabledLayerCount = validationLayers.size();
		return createInfo;
	}();

	m_device = m_physicalDevice.createDeviceUnique(createInfo);
	m_deviceWaitIdle.device = m_device.get();
	m_queue = m_device->getQueue(m_queueFamily.index, 0);
}

////////////////////////////////
void VulkanResources::createRenderTextureImages(const uint32_t nImages, const uint32_t width, const uint32_t height)
{
	const int numLayers{1};
	const vk::Format format{vk::Format::eR8G8B8A8Unorm};
	const vk::ImageUsageFlags usage{vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled};
	const vk::Extent2D extent{width, height};
	const vk::ImageTiling tiling = vk::ImageTiling::eOptimal;

	m_renderTextureImageFormat = format;
	m_renderTextureExtent = extent;
	m_renderTextureImages.reserve(nImages);

	for (auto i = 0U; i < nImages; ++i)
	{
		m_renderTextureImages.emplace_back(createTextureImage(m_device.get(), extent, numLayers, format, tiling, usage));
	}
}

void VulkanResources::createRenderTextureDeviceMemories()
{
	const auto nImages = m_renderTextureImages.size();
	const auto devMemProps = m_physicalDevice.getMemoryProperties();
	const auto memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	m_renderTextureDeviceMemories.reserve(nImages);
	for (const auto& image : m_renderTextureImages)
	{
		const auto memReqs = m_device->getImageMemoryRequirements(image.get());
		m_renderTextureDeviceMemories.emplace_back(createTextureDeviceMemory(m_device.get(), image.get(), memReqs, devMemProps, memProps));
	}
}

void VulkanResources::createRenderTextureImageViews()
{
	const int numLayers{1};
	const vk::Format format{m_renderTextureImageFormat};
	const vk::ImageViewType viewType{vk::ImageViewType::e2D};

	const auto nImages = m_renderTextureImages.size();
	m_renderTextureImageViews.reserve(nImages);
	for (auto i = 0U; i < nImages; ++i)
	{
		const auto image = m_renderTextureImages[i].get();
		const auto deviceMemory = m_renderTextureDeviceMemories[i].get();
		const auto memReqs = m_device->getImageMemoryRequirements(image);
		const auto memOffset = 0 * memReqs.alignment;
		m_device->bindImageMemory(image, deviceMemory, memOffset);
		m_renderTextureImageViews.emplace_back(createTextureImageView(m_device.get(), image, numLayers, viewType, format));
	}
}

void VulkanResources::createRenderTextureRenderPass()
{
	m_renderTextureRenderPass = createSimpleRenderPass(m_device.get(), m_renderTextureImageFormat, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VulkanResources::createRenderTextureDescriptorSetLayout()
{
	const uint32_t uboCount{1};
	const uint32_t samplerCount{0};
	m_renderTextureDescriptorSetLayout = createDescriptorSetLayout(m_device.get(), uboCount, samplerCount);
}

void VulkanResources::createRenderTextureGraphicsPipeline()
{
	const vector<unsigned char> vertShaderCode = {begin(vlk_shaderDemoSceneVert), end(vlk_shaderDemoSceneVert)};
	const vector<unsigned char> fragShaderCode = {begin(vlk_shaderDemoSceneFrag), end(vlk_shaderDemoSceneFrag)};
	const vector<vk::DescriptorSetLayout> setLayouts = {m_renderTextureDescriptorSetLayout.get()};
	m_renderTexturePipelineLayout = createSimpleGraphicsPipelineLayout(m_device.get(), setLayouts);

	const auto bindingDescription = RenderTextureVertex::getBindingDescription();
	const auto attributeDescriptions = RenderTextureVertex::getAttributeDescriptions();
	const vk::PipelineVertexInputStateCreateInfo vertexInputState = [&bindingDescription, &attributeDescriptions] {
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.flags = {};
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		return vertexInputInfo;
	}();
	m_renderTextureGraphicsPipeline = createSimpleGraphicsPipeline(
		m_device.get(),
		vertShaderCode,
		fragShaderCode,
		m_renderTextureExtent,
		m_renderTexturePipelineLayout.get(),
		m_renderTextureRenderPass.get(),
		vertexInputState);
}

void VulkanResources::createRenderTextureFramebuffers()
{
	m_renderTextureFramebuffers = createFramebuffers(m_device.get(), m_renderTextureImageViews, m_renderTextureRenderPass.get(), m_renderTextureExtent);
}

void VulkanResources::createRenderTextureVertexBuffer(const Span<const RenderTextureVertex> verts)
{
	auto bufAndMem = createVertexBuffer(m_physicalDevice, m_device.get(), m_commandPool.get(), m_queueFamily.index, m_queue, verts);
	m_renderTextureVertexBuffer = std::move(bufAndMem.buffer);
	m_renderTextureVertexBufferMemory = std::move(bufAndMem.deviceMemory);
}

void VulkanResources::createRenderTextureIndexBuffer(const Span<const RenderTextureVertex::IndexType> inds)
{
	auto bufAndMem = createIndexBuffer(m_physicalDevice, m_device.get(), m_commandPool.get(), m_queueFamily.index, m_queue, inds);
	m_renderTextureIndexBuffer = std::move(bufAndMem.buffer);
	m_renderTextureIndexBufferMemory = std::move(bufAndMem.deviceMemory);
}

void VulkanResources::createRenderTextureDescriptorPool()
{
	const uint32_t nImages = 2U * m_renderTextureImages.size();
	array<vk::DescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
	poolSizes[0].descriptorCount = nImages;

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = nImages;

	m_renderTextureDescriptorPool = m_device->createDescriptorPoolUnique(poolInfo);
}

void VulkanResources::createRenderTextureDescriptorSets()
{
	const uint32_t nImages2 = 2U * m_renderTextureImages.size(); // left/right
	const vector<vk::DescriptorSetLayout> layouts(nImages2, m_renderTextureDescriptorSetLayout.get());
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = m_renderTextureDescriptorPool.get();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(nImages2);
	allocInfo.pSetLayouts = layouts.data();

	m_renderTextureDescriptorSets = m_device->allocateDescriptorSetsUnique(allocInfo);

	for (auto i = 0u; i < nImages2; ++i)
	{
		array<vk::DescriptorBufferInfo, 1> bufInfo;
		bufInfo[0].buffer = m_renderTextureUniformBuffers[i].get();
		bufInfo[0].offset = 0;
		bufInfo[0].range = sizeof(RenderTextureUbo);

		array<vk::WriteDescriptorSet, 1> descriptorWrites;
		descriptorWrites[0].dstSet = m_renderTextureDescriptorSets[i].get();
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = bufInfo.size();
		descriptorWrites[0].pBufferInfo = bufInfo.data();
		m_device->updateDescriptorSets(descriptorWrites, nullptr);
	}
}

////////////////////////////////
void VulkanResources::createSwapchain(const uint32_t width, const uint32_t height)
{
	const SwapchainSupportDetails swapchainSupport = querySwapchainSupport(m_physicalDevice, m_surface.get());
	const vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(swapchainSupport.formats);
	const vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(swapchainSupport.presentModes);
	const vk::Extent2D extent = chooseSwapchainExtent(swapchainSupport.capabilities, width, height);

	const vk::SwapchainCreateInfoKHR createInfo =
		[this, &swapchainSupport, &surfaceFormat, &presentMode, &extent] {
			uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
			{
				if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
				{
					imageCount = swapchainSupport.capabilities.maxImageCount;
				}
			}
			vk::SwapchainCreateInfoKHR createInfo{};
			createInfo.surface = m_surface.get();
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
			createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = nullptr;
			const auto queueFamilyIndices = array<uint32_t, 1>{
				m_queueFamily.index};
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
			createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
			return createInfo;
		}();

	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
	m_swapchain = m_device->createSwapchainKHRUnique(createInfo);
}

void VulkanResources::createSwapchainImages()
{
	m_swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain.get());
}

void VulkanResources::createSwapchainImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());

	const vk::ComponentMapping components{
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
	};
	const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	for (size_t i = 0; i < m_swapchainImages.size(); i++)
	{
		const vk::ImageViewCreateInfo createInfo = [this, i, &components, &subresourceRange] {
			vk::ImageViewCreateInfo createInfo;
			createInfo.image = m_swapchainImages[i];
			createInfo.viewType = vk::ImageViewType::e2D;
			createInfo.format = m_swapchainImageFormat;
			createInfo.components = components;
			createInfo.subresourceRange = subresourceRange;
			return createInfo;
		}();
		m_swapchainImageViews[i] = m_device->createImageViewUnique(createInfo);
	}
}

void VulkanResources::createSwapchainRenderPass()
{
	m_swapchainRenderPass = createSimpleRenderPass(m_device.get(), m_swapchainImageFormat, vk::ImageLayout::ePresentSrcKHR);
}

void VulkanResources::createSwapchainDescriptorSetLayout()
{
	const uint32_t uboCount{0};
	const uint32_t samplerCount{1};
	m_swapchainDescriptorSetLayout = createDescriptorSetLayout(m_device.get(), uboCount, samplerCount);
}

void VulkanResources::createSwapchainGraphicsPipeline()
{
	const vector<unsigned char> vertShaderCode = {begin(vlk_shaderTextureCopyVert), end(vlk_shaderTextureCopyVert)};
	const vector<unsigned char> fragShaderCode = {begin(vlk_shaderTextureCopyFrag), end(vlk_shaderTextureCopyFrag)};
	const vector<vk::DescriptorSetLayout> setLayouts = {m_swapchainDescriptorSetLayout.get()};
	m_swapchainPipelineLayout = createSimpleGraphicsPipelineLayout(m_device.get(), setLayouts);

	const auto bindingDescription = SwapchainVertex::getBindingDescription();
	const auto attributeDescriptions = SwapchainVertex::getAttributeDescriptions();
	const vk::PipelineVertexInputStateCreateInfo vertexInputState = [&bindingDescription, &attributeDescriptions] {
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.flags = {};
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		return vertexInputInfo;
	}();

	m_swapchainGraphicsPipeline = createSimpleGraphicsPipeline(
		m_device.get(),
		vertShaderCode,
		fragShaderCode,
		m_swapchainExtent,
		m_swapchainPipelineLayout.get(),
		m_swapchainRenderPass.get(),
		vertexInputState);
}

void VulkanResources::createSwapchainFramebuffers()
{
	m_swapchainFramebuffers = createFramebuffers(m_device.get(), m_swapchainImageViews, m_swapchainRenderPass.get(), m_swapchainExtent);
}

void VulkanResources::createSwapchainVertexBuffer(const Span<const SwapchainVertex> verts)
{
	auto bufAndMem = createVertexBuffer(m_physicalDevice, m_device.get(), m_commandPool.get(), m_queueFamily.index, m_queue, verts);
	m_swapchainVertexBuffer = std::move(bufAndMem.buffer);
	m_swapchainVertexBufferMemory = std::move(bufAndMem.deviceMemory);
}

void VulkanResources::createSwapchainIndexBuffer(const Span<const SwapchainVertex::IndexType> inds)
{
	auto bufAndMem = createIndexBuffer(m_physicalDevice, m_device.get(), m_commandPool.get(), m_queueFamily.index, m_queue, inds);
	m_swapchainIndexBuffer = std::move(bufAndMem.buffer);
	m_swapchainIndexBufferMemory = std::move(bufAndMem.deviceMemory);
}

void VulkanResources::createSwapchainTextureSampler()
{
	const vk::Device device = m_device.get();
	m_swapchainTextureSampler = createTextureSampler(device);
}

void VulkanResources::createSwapchainUniformBuffers()
{
	const auto nImages = m_swapchainImages.size();
	const vk::DeviceSize bufferSize = sizeof(SwapchainUbo);

	auto res = createBuffersAndMemories(
		m_physicalDevice,
		m_device.get(),
		nImages,
		bufferSize,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		m_queueFamily.index);
	m_swapchainUniformBuffers = std::move(res.buffers);
	m_swapchainUniformBufferMemories = std::move(res.deviceMemories);
}

void VulkanResources::createRenderTextureUniformBuffers()
{
	const auto nImages2 = 2U * m_renderTextureImages.size(); // left/right
	const vk::DeviceSize bufferSize = sizeof(RenderTextureUbo);

	auto res = createBuffersAndMemories(
		m_physicalDevice,
		m_device.get(),
		nImages2,
		bufferSize,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		m_queueFamily.index);
	m_renderTextureUniformBuffers = std::move(res.buffers);
	m_renderTextureUniformBufferMemories = std::move(res.deviceMemories);
}

void VulkanResources::createSwapchainDescriptorPool()
{
	const uint32_t nImages = m_swapchainImages.size();
	array<vk::DescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = vk::DescriptorType::eCombinedImageSampler;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(nImages);

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = nImages;

	m_swapchainDescriptorPool = m_device->createDescriptorPoolUnique(poolInfo);
}

// This refers to textures from the render texture
void VulkanResources::createSwapchainDescriptorSets()
{
	const uint32_t nImages = m_swapchainImages.size();
	const vector<vk::DescriptorSetLayout> layouts(nImages, m_swapchainDescriptorSetLayout.get());
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = m_swapchainDescriptorPool.get();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(nImages);
	allocInfo.pSetLayouts = layouts.data();

	m_swapchainDescriptorSets = m_device->allocateDescriptorSetsUnique(allocInfo);

	for (auto i = 0u; i < nImages; ++i)
	{
		vector<vk::WriteDescriptorSet> descriptorWrites(1);

		vk::DescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imgInfo.imageView = m_renderTextureImageViews[i].get(); // texture from render texture pass
		imgInfo.sampler = m_swapchainTextureSampler.get();

		descriptorWrites[0].dstSet = m_swapchainDescriptorSets[i].get();
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imgInfo;

		m_device->updateDescriptorSets(descriptorWrites, nullptr);
	}
}

////////////////////////////////
void VulkanResources::createCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = {};
	poolInfo.queueFamilyIndex = m_queueFamily.index;
	m_commandPool = m_device->createCommandPoolUnique(poolInfo);
}

void VulkanResources::createCommandBuffers(const uint32_t nImages)
{
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = m_commandPool.get();
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = nImages;

	m_commandBuffers = m_device->allocateCommandBuffersUnique(allocInfo);
}

void VulkanResources::createSyncObjects(const uint32_t nImages, const uint32_t nMaxFramesInFlight)
{
	m_imageAvailableSemaphores.reserve(nMaxFramesInFlight);
	m_renderFinishedSemaphores.reserve(nMaxFramesInFlight);
	m_inFlightFences.reserve(nMaxFramesInFlight);
	for (size_t i = 0; i < nMaxFramesInFlight; ++i)
	{
		m_imageAvailableSemaphores.emplace_back(m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
		m_renderFinishedSemaphores.emplace_back(m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
		vk::FenceCreateInfo fenceCreateInfo;
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		m_inFlightFences.emplace_back(m_device->createFenceUnique(fenceCreateInfo));
	}

	m_imageInUseFences.resize(nImages, nullptr); // refers to m_inFlightFences

	m_waitAllFences = [this, device = m_device.get()] {
		vector<vk::Fence> fences;
		for (const auto& fence : m_inFlightFences)
		{
			fences.emplace_back(fence.get());
		}
		return VulkanWaitAllFences{device, std::move(fences)};
	}();
}

void VulkanResources::cleanupSwapchain()
{
	const vk::Device device = m_device.get();
	m_swapchainFramebuffers.clear();
	m_commandBuffers.clear();
	m_swapchainGraphicsPipeline.reset();
	m_swapchainPipelineLayout.reset();
	m_swapchainRenderPass.reset();
	m_swapchainImageViews.clear();
	m_swapchainImages.clear();
	m_swapchain.reset();

	for (auto i = 0u; i < m_swapchainImages.size(); ++i)
	{
		device.destroyBuffer(m_swapchainUniformBuffers[i].release());
		device.freeMemory(m_swapchainUniformBufferMemories[i].release());
	}
	m_swapchainUniformBuffers.clear();
	m_swapchainUniformBufferMemories.clear();

	m_swapchainDescriptorSets.clear();
	m_swapchainDescriptorPool.reset();
}

void VulkanResources::recreateSwapchain(NativeWindow& nativeWindow)
{
	const auto windowSize = nativeWindow.windowSize();
	cout << "Next window size: " << windowSize.width << "x" << windowSize.height << '\n'
		 << flush;

	m_device->waitIdle();

	cleanupSwapchain();

	createSwapchain(windowSize.width, windowSize.height);
	createSwapchainImages();
	createSwapchainImageViews();
	createSwapchainRenderPass();
	createSwapchainGraphicsPipeline();
	createSwapchainFramebuffers();
	createSwapchainUniformBuffers();
	createSwapchainDescriptorPool();
	createSwapchainDescriptorSets();

	const uint32_t nImages = m_swapchainImages.size();
	createCommandBuffers(nImages);
	recordCommandBuffers(Span<const RenderTextureVertex>{levelModelVerts}, Span<const SwapchainVertex::IndexType>{g_indices2});
}

void VulkanResources::updateRenderTextureUniformBuffer(const uint32_t index, const RenderTextureUbo& ubo)
{
	void* data;
	const vk::Device device{m_device.get()};
	const vk::DeviceSize offset{0};
	const vk::DeviceSize size{sizeof(ubo)};
	const auto ret = device.mapMemory(
		m_renderTextureUniformBufferMemories[index].get(), offset, size, vk::MemoryMapFlags{}, &data);
	if (ret != vk::Result::eSuccess)
	{
		throw "Map uniform buffer";
	}
	memcpy(data, &ubo, sizeof(ubo));
	device.unmapMemory(m_renderTextureUniformBufferMemories[index].get());
}

void VulkanResources::updateSwapchainUniformBuffer()
{
	// empty
}
////////////////////////////////
void VulkanResources::recordCommandBuffers(const Span<const RenderTextureVertex> sceneVerts, const Span<const SwapchainVertex::IndexType> quadInds)
{
	for (size_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		const vk::CommandBuffer commandBuffer = m_commandBuffers[i].get();
		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		commandBuffer.begin(beginInfo);

		// render texture pass
		{
			const uint32_t halfWidth = m_renderTextureExtent.width / 2;
			const uint32_t height = m_renderTextureExtent.height;

			const vk::ClearColorValue clearColorValue{array<float, 4>{0.3F, 0.3F, 0.8F, 0.3F}};
			const vk::ClearValue clearColor{clearColorValue};

			vk::RenderPassBeginInfo renderPassInfo;
			renderPassInfo.renderPass = m_renderTextureRenderPass.get();
			renderPassInfo.framebuffer = m_renderTextureFramebuffers[i].get();
			renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
			renderPassInfo.renderArea.extent = m_renderTextureExtent;
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;
			commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			// For each left/right eyes
			for (int32_t j = 0; j < 2; ++j)
			{
				const vk::Viewport currentViewport{static_cast<float>(halfWidth * j), 0, static_cast<float>(halfWidth), static_cast<float>(height), 0.0F, 1.0F};
				const vk::Rect2D currentScissor{{static_cast<int32_t>(halfWidth * j), 0}, {halfWidth, height}};

				const array<vk::Buffer, 1> vertexBuffers = {m_renderTextureVertexBuffer.get()};
				vk::DeviceSize offsets[] = {0};

				commandBuffer.setViewport(0, currentViewport);
				commandBuffer.setScissor(0, currentScissor);
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_renderTexturePipelineLayout.get(), 0, m_renderTextureDescriptorSets[2 * i + j].get(), nullptr);
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderTextureGraphicsPipeline.get());
				commandBuffer.bindVertexBuffers(0, vertexBuffers.size(), vertexBuffers.data(), offsets);
				commandBuffer.draw(sceneVerts.size(), 1, 0, 0);
			}
			commandBuffer.endRenderPass();
		}

		// render to debug screen
		{
			const vk::ClearColorValue clearColorValue{array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f}};
			const vk::ClearValue clearColor{clearColorValue};
			vk::RenderPassBeginInfo renderPassInfo;
			renderPassInfo.renderPass = m_swapchainRenderPass.get();
			renderPassInfo.framebuffer = m_swapchainFramebuffers[i].get();
			renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
			renderPassInfo.renderArea.extent = m_swapchainExtent;
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			const vk::Viewport currentViewport{0, 0, static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F};
			const vk::Rect2D currentScissor{{0, 0}, {m_swapchainExtent.width, m_swapchainExtent.height}};

			const array<vk::Buffer, 1> vertexBuffers = {m_swapchainVertexBuffer.get()};
			const vk::Buffer indexBuffer = m_swapchainIndexBuffer.get();
			vk::DeviceSize offsets[] = {0};

			commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			commandBuffer.setViewport(0, currentViewport);
			commandBuffer.setScissor(0, currentScissor);
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_swapchainPipelineLayout.get(), 0, m_swapchainDescriptorSets[i].get(), nullptr);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_swapchainGraphicsPipeline.get());
			commandBuffer.bindVertexBuffers(0, vertexBuffers.size(), vertexBuffers.data(), offsets);
			commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
			commandBuffer.drawIndexed(quadInds.size(), 1, 0, 0, 0);
			commandBuffer.endRenderPass();
		}

		commandBuffer.end();
	}
}

uint32_t VulkanResources::drawFrame(NativeWindow& nativeWindow, const RenderTextureUboLR& ubo)
{
	const auto currentFrame = m_currentFrame;
	if (const auto res = m_device->waitForFences(m_inFlightFences[currentFrame].get(), true, UINT64_MAX); res != vk::Result::eSuccess)
	{
		throw "Failed to wait for fences";
	}

	vk::ResultValue<uint32_t> result = m_device->acquireNextImageKHR(
		m_swapchain.get(), UINT64_MAX, m_imageAvailableSemaphores[currentFrame].get(), {});
	if (result.result == vk::Result::eErrorOutOfDateKHR)
	{
		recreateSwapchain(nativeWindow);
	}
	if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
	{
		throw "Failed to acquire next image!";
	}

	const uint32_t imageIndex = result.value;
	updateRenderTextureUniformBuffer(2U * imageIndex + 0U, ubo.uboL); // left
	updateRenderTextureUniformBuffer(2U * imageIndex + 1U, ubo.uboR); // right

	if (m_imageInUseFences[imageIndex] != vk::Fence{nullptr})
	{
		const auto res = m_device->waitForFences(m_imageInUseFences[imageIndex], true, UINT64_MAX);
		if (res != vk::Result::eSuccess)
		{
			throw "Failed to wait for fences";
		}
	}
	m_imageInUseFences[imageIndex] = m_inFlightFences[currentFrame].get();

	vk::PipelineStageFlags waitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[currentFrame].get();
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[currentFrame].get();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex].get();

	m_device->resetFences(m_inFlightFences[currentFrame].get());
	m_queue.submit(submitInfo, m_inFlightFences[currentFrame].get());

	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[currentFrame].get();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain.get();
	presentInfo.pImageIndices = &imageIndex;

	bool outOfDate{m_swapchainFramebufferResized ? true : false};
	try
	{
		const auto res2 = m_queue.presentKHR(presentInfo);
		if (res2 == vk::Result::eErrorOutOfDateKHR || res2 == vk::Result::eSuboptimalKHR)
		{
			outOfDate = true;
		}
		else if (res2 != vk::Result::eSuccess)
		{
			throw "Failed to present swap chain image";
		}
	}
	catch (...)
	{
		// presentKHR can throw as well..
		outOfDate = true;
	}
	if (outOfDate)
	{
		cerr << "Recreating swapchain\n";
		m_swapchainFramebufferResized = false;
		recreateSwapchain(nativeWindow);
	}

	m_currentFrame = (currentFrame + 1) % N_MAX_FRAMES_IN_FLIGHT;
	return imageIndex;
}

class VulkanExample
{
public:
	VulkanExample() = default;
	~VulkanExample() noexcept = default;

	VulkanExample(VulkanExample&&) = delete;
	VulkanExample& operator=(VulkanExample&&) = delete;
	VulkanExample(const VulkanExample&) = delete;
	VulkanExample& operator=(const VulkanExample&) = delete;

	void initVulkan(NativeWindow&);
	void initRenderTexturePipeline(const uint32_t nImages, const uint32_t width, const uint32_t height, Span<const RenderTextureVertex>);
	void initSwapchainPipeline(const uint32_t nImages, Span<const SwapchainVertex>, Span<const SwapchainVertex::IndexType>);
	void initCommandBuffers(const uint32_t nImages, const uint32_t nMaxFramesInFlight);

	uint32_t nSwapchainImages() const; // valid after initVulkan()
	uint32_t draw(const RenderTextureUboLR&);

	const Fove::VulkanTexture texture(const uint32_t index) const
	{
		Fove::VulkanTexture tex{
			textureContext(),
			textureResources(index),
			m_vulkan.m_renderTextureExtent.width,
			m_vulkan.m_renderTextureExtent.height,
		};
		return tex;
	}

private:
	vk::Instance instance() const { return m_vulkan.m_instance.get(); }
	vk::SurfaceKHR surface() const { return m_vulkan.m_surface.get(); }
	vk::PhysicalDevice physicalDevice() const { return m_vulkan.m_physicalDevice; }
	vk::Device device() const { return m_vulkan.m_device.get(); }
	vk::Queue queue() const { return m_vulkan.m_queue; }
	QueueFamily queueFamily() const { return m_vulkan.m_queueFamily; }
	vk::SwapchainKHR swapchain() const { return m_vulkan.m_swapchain.get(); }
	const vector<vk::Image>& swapchainImages() const { return m_vulkan.m_swapchainImages; }

	void mainLoop();

	Fove::VulkanContext textureContext() const
	{
		Fove::VulkanContext context{
			instance(),
			physicalDevice(),
			device(),
			queue(),
			queue(),
			queue(),
			queueFamily().index,
			queueFamily().index,
			queueFamily().index,
		};
		return context;
	}

	const Fove::VulkanTextureResources textureResources(const uint32_t index) const
	{
		Fove::VulkanTextureResources resources{
			m_vulkan.m_renderTextureDeviceMemories[index].get(),
			m_vulkan.m_renderTextureImages[index].get(),
			m_vulkan.m_renderTextureImageViews[index].get(),
		};
		return resources;
	};

private:
	NativeWindow* m_nativeWindow{nullptr};
	VulkanResources m_vulkan{};
};

// Need to setup your own Vulkan context, apart from the one that Fove SDK uses.
void VulkanExample::initVulkan(NativeWindow& nativeWindow)
{
	m_nativeWindow = &nativeWindow;
	// get the instance independent function pointers
	static const vk::DynamicLoader dl;
	{
		// Manually load some Vulkan API function addresses
		const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
	}

	m_vulkan.createInstance();
	if (m_vulkan.m_enableDebugUtils)
		m_vulkan.setupDebugMessenger();
	m_vulkan.createXlibSurface(nativeWindow.xDisplay(), nativeWindow.xWindow());
	m_vulkan.pickPhysicalDevice();
	m_vulkan.pickQueue();
	m_vulkan.createLogicalDevice();

	const auto size = nativeWindow.windowSize();
	m_vulkan.createSwapchain(static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height));
	m_vulkan.createSwapchainImages();
	m_vulkan.createSwapchainImageViews();

	// Since we are sloppy and using same command pool for everything,
	// we need to create the command pool early
	m_vulkan.createCommandPool();

	cout << "Vulkan:\n"
		 << "- Instance: " << m_vulkan.m_instance.get() << '\n'
		 << "- Physical device: " << m_vulkan.m_physicalDevice << '\n'
		 << "- Logical device: " << m_vulkan.m_device.get() << '\n'
		 << "- Queue family index: " << m_vulkan.m_queueFamily.index << '\n'
		 << "- Queue: " << m_vulkan.m_queue << '\n'
		 << "- Swapchain:" << m_vulkan.m_swapchain.get() << '\n'
		 << "- Command pool:" << m_vulkan.m_commandPool.get() << '\n'
		 << '\n'
		 << flush;
}

void VulkanExample::initRenderTexturePipeline(const uint32_t nImages, const uint32_t width, const uint32_t height, const Span<const RenderTextureVertex> verts)
{
	m_vulkan.createRenderTextureImages(nImages, width, height);
	m_vulkan.createRenderTextureDeviceMemories();
	m_vulkan.createRenderTextureImageViews();
	m_vulkan.createRenderTextureRenderPass();
	m_vulkan.createRenderTextureDescriptorSetLayout();
	m_vulkan.createRenderTextureGraphicsPipeline();
	m_vulkan.createRenderTextureFramebuffers();
	m_vulkan.createRenderTextureVertexBuffer(verts);
	m_vulkan.createRenderTextureUniformBuffers();
	m_vulkan.createRenderTextureDescriptorPool();
	m_vulkan.createRenderTextureDescriptorSets();
}

void VulkanExample::initSwapchainPipeline(const uint32_t nImages, const Span<const SwapchainVertex> verts, const Span<const SwapchainVertex::IndexType> inds)
{
	m_vulkan.createSwapchainRenderPass();
	m_vulkan.createSwapchainDescriptorSetLayout();
	m_vulkan.createSwapchainGraphicsPipeline();
	m_vulkan.createSwapchainFramebuffers();
	m_vulkan.createSwapchainVertexBuffer(verts);
	m_vulkan.createSwapchainIndexBuffer(inds);
	m_vulkan.createSwapchainTextureSampler();
	m_vulkan.createSwapchainUniformBuffers();
	m_vulkan.createSwapchainDescriptorPool();
	m_vulkan.createSwapchainDescriptorSets();
}

void VulkanExample::initCommandBuffers(const uint32_t nImages, const uint32_t nMaxFramesInFlight)
{
	m_vulkan.createCommandBuffers(nImages);
	m_vulkan.createSyncObjects(nImages, nMaxFramesInFlight);

	m_vulkan.recordCommandBuffers(Span<const RenderTextureVertex>{levelModelVerts}, Span<const SwapchainVertex::IndexType>{g_indices2});
}

uint32_t VulkanExample::nSwapchainImages() const
{
	return static_cast<uint32_t>(m_vulkan.m_swapchainImages.size());
}

uint32_t VulkanExample::draw(const RenderTextureUboLR& ubo)
{
	return m_vulkan.drawFrame(*m_nativeWindow, ubo);
}

int run(NativeLaunchInfo info)
try
{
	// Connect to headset, specifying the capabilities we will use.
	// NOTE:
	// Result<T>::getValue() will throw on error, so we do not explicitly check return values.
	// In real applications, you probably wants to do more proper error handling.
	Fove::Headset headset = Fove::Headset::create(Fove::ClientCapabilities::OrientationTracking | Fove::ClientCapabilities::PositionTracking | Fove::ClientCapabilities::EyeTracking | Fove::ClientCapabilities::GazedObjectDetection).getValue();

	// Create a window and setup a Vulkan instance associated with it
	NativeWindow nativeWindow = createNativeWindow(info, appName);
	VulkanExample app{};
	app.initVulkan(nativeWindow);

	// Connect to compositor
	// The compositor uses the vulkan context so it should be killed before (and thus created after)
	Fove::Compositor compositor = headset.createCompositor().getValue();

	// Create a compositor layer, which we will use for submission
	const Fove::CompositorLayerCreateInfo layerCreateInfo{}; // Using all default values
	Fove::Result<Fove::CompositorLayer> layerOrError = compositor.createLayer(layerCreateInfo);
	const Fove::Vec2i resolutionPerEye = layerOrError ? layerOrError->idealResolutionPerEye : Fove::Vec2i{1024, 1024};

	// The main rendering logic:
	// (1) Render to a texture - this would be the main VR content
	// (2) Submit the rendereed texture to Fove - Fove runtime will then render the texture on the headset display
	// (3) Render the same texture to window - this is optional and merely for monitoring/debugging
	// Set up a framebuffer which we will render to
	// If we were unable to create a layer now (eg. compositor not running), use a default size while we wait for the compositor

	// Prepare gpu resources needed for rendering
	const uint32_t nImages = app.nSwapchainImages();
	app.initRenderTexturePipeline(nImages, 2 * resolutionPerEye.x, resolutionPerEye.y, Span<const RenderTextureVertex>{levelModelVerts});
	app.initSwapchainPipeline(nImages, Span<const SwapchainVertex>{g_vertices2}, Span<const SwapchainVertex::IndexType>{g_indices2});
	// Define the rendering logic by pre-recording to command buffers
	app.initCommandBuffers(nImages, N_MAX_FRAMES_IN_FLIGHT);

	// Register all objects with FOVE SceneAware
	// This allows FOVE to handle all the detection of which object you're looking at
	// Object picking can be done manually if needed, using the gaze vectors
	// However, we recommending using the FOVE API, as the additional scene info can increase the accuracy of ET
	constexpr int cameraId = 9999; // Any arbitrary int not used by the objects
	{
		// Setup camera
		// Posiiton will be updated each frame in the main loop
		Fove::CameraObject cam;
		cam.id = cameraId;
		checkError(headset.registerCameraObject(cam), "registerCameraObject");

		// This can also be done manually if needed, using the gaze vectors,
		// but we recommend using the FOVE API, as the additional scene info can increase the accuracy of ET
		constexpr size_t numSphereFloats = sizeof(collisionSpheres) / sizeof(float);
		static_assert(numSphereFloats % 5 == 0, "Invalid collision sphere format");
		constexpr size_t numSpheres = numSphereFloats / 5;
		for (size_t i = 0; i < numSpheres; ++i)
		{
			Fove::ObjectCollider collider;
			collider.center = Fove::Vec3{collisionSpheres[i * 5 + 2], collisionSpheres[i * 5 + 3], collisionSpheres[i * 5 + 4]};
			collider.shapeType = Fove::ColliderType::Sphere;
			collider.shapeDefinition.sphere.radius = collisionSpheres[i * 5 + 1];

			Fove::GazableObject object;
			object.colliderCount = 1;
			object.colliders = &collider;
			object.group = Fove::ObjectGroup::Group0; // Groups allows masking of different objects to difference cameras (not needed here)
			object.id = static_cast<int>(collisionSpheres[i * 5 + 0]);
			checkError(headset.registerGazableObject(object), "registerGazableObject");
		}
	}

	while (true)
	{
		// Update ubo and selected model
		RenderTextureUboLR ubo{};
		{
			if (!flushWindowEvents(nativeWindow))
				break;

			// Create layer if we have none
			// This allows us to connect to the compositor once it launches
			if (!layerOrError)
			{
				// Check if the compositor is ready first. Otherwise we will hang for a while when trying to create a layer
				Fove::Result<bool> isReadyOrError = compositor.isReady();
				if (isReadyOrError.isValid() && isReadyOrError.getValue())
				{
					if ((layerOrError = compositor.createLayer(layerCreateInfo)).isValid())
					{
						// TODO resize rendering surface
					}
				}
			}

			// Determine the selection object based on what's being gazed at
			headset.fetchEyeTrackingData();
			if (const Fove::Result<int> gazeOrError = headset.getGazedObjectId(); gazeOrError && gazeOrError.getValue() != fove_ObjectIdInvalid)
			{
				ubo.uboL.selection = static_cast<float>(gazeOrError.getValue());
				ubo.uboR.selection = ubo.uboL.selection;
			}
		}

		// Wait for the compositor to tell us to render
		// This allows the compositor to limit our frame rate to what's appropriate for the HMD display
		// We move directly on to rendering after this, the update phase happens before hand
		// This is to ensure the quickest possible turnaround time from being signaled to presenting a frame,
		// such that we reduce the risk of missing a frame due to time spent during update
		const Fove::Result<Fove::Pose> poseOrError = compositor.waitForRenderPose();
		const Fove::Pose pose = poseOrError.isValid() ? poseOrError.getValue() : Fove::Pose();
		if (poseOrError.isValid())
		{
			// If there was an error waiting, it's possible that WaitForRenderPose returned immediately
			// Sleep a little bit to prevent us from rendering at maximum framerate and eating massive resources/battery
			this_thread::sleep_for(10ms);
		}

		// Prepare uniforms
		{
			// Compute the modelview matrix
			// Everything here is reverse since we are moving the world we are going to draw, not the camera
			const Fove::Matrix44 modelView = quatToMatrix(conjugate(pose.orientation))                                 // Apply the HMD orientation
											 * translationMatrix(-pose.position.x, -pose.position.y, -pose.position.z) // Apply the position tracking offset
											 * translationMatrix(0, -playerHeight, 0);                                 // Move ground downwards to compensate for player height
			// Adjust clipspace coordinates
			const Fove::Matrix44 glToVk = {{
				{1.0F, 0.0F, 0.0F, 0.0F},
				{0.0F, -1.0F, 0.0F, 0.0F}, // y to -y
				{0.0F, 0.0F, 0.5F, 0.5F},  // adjust z clip
				{0.0F, 0.0F, 0.0F, 1.0F},
			}};

			// Get distance between eyes to shift camera for stereo effect
			const Fove::Result<float> iodOrError = headset.getRenderIOD();
			const float halfIOD = 0.5f * (iodOrError.isValid() ? iodOrError.getValue() : 0.064f);

			// Fetch the projection matrices
			Fove::Result<Fove::Stereo<Fove::Matrix44>> projectionsOrError = headset.getProjectionMatricesLH(0.01f, 1000.0f);
			if (projectionsOrError.isValid())
			{
				ubo.uboL.mvp = glToVk * transpose(projectionsOrError->l) * translationMatrix(+halfIOD, 0, 0) * modelView;
				ubo.uboR.mvp = glToVk * transpose(projectionsOrError->r) * translationMatrix(-halfIOD, 0, 0) * modelView;
				// Render the scene twice, once for the left, once for the right
			}
		}

		// Render the scene to the texture and present it to the host screen
		const auto index = app.draw(ubo);

		// Present rendered results to compositor
		if (layerOrError)
		{
			const Fove::VulkanTexture tex = app.texture(index);

			Fove::CompositorLayerSubmitInfo submitInfo{};
			submitInfo.layerId = layerOrError->layerId;
			submitInfo.pose = pose;
			submitInfo.left.texInfo = &tex;
			submitInfo.right.texInfo = &tex;

			Fove::TextureBounds bounds{};
			bounds.top = 0.0F;
			bounds.bottom = 1.0F;
			bounds.left = 0.0F;
			bounds.right = 0.5F;
			submitInfo.left.bounds = bounds;
			bounds.left = 0.5F;
			bounds.right = 1.0F;
			submitInfo.right.bounds = bounds;

			compositor.submit(submitInfo); // Error ignored, just continue rendering to the window when we're disconnected
		}

		// Update camera position used by FOVE gaze detection
		Fove::ObjectPose camPose;
		camPose.position = pose.position;
		camPose.position.y += playerHeight;
		camPose.velocity = pose.velocity;
		camPose.rotation = pose.orientation;
		checkError(headset.updateCameraObject(cameraId, camPose), "updateCameraObject");
	}

	return 0;
}
catch (...)
{
	showErrorBox("Error: " + currentExceptionMessage());
	return -1;
}

} // namespace

void programMain(NativeLaunchInfo info)
{
	const int ret = run(std::move(info));
	if (ret != 0)
	{
		cerr << "VulkanExample exitted abnormally\n";
	}
	return;
}
