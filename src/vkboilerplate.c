#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkloadextensions(VkBoilerplate* bp)
{	
	vkCreateDebugUtilsMessengerEXTproxy =
		(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			bp->inst, "vkCreateDebugUtilsMessengerEXT"
	);

    vkDestroyDebugUtilsMessengerEXTproxy =
		(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			bp->inst, "vkDestroyDebugUtilsMessengerEXT"
	);
	
	logi("vulkan extensions loaded\n");
}

void vkboilerplatec(VkBoilerplate* bp, Window* win)
{
	UPDATE_DEBUG_FILE();
	
	const char* extension_names[] = {
		"VK_EXT_debug_utils", "VK_KHR_surface", "VK_KHR_win32_surface"
	};
	const char* layer_names[] = { "VK_LAYER_KHRONOS_validation" };

	bp->user_data = (dUserData) { __LINE__, __FILE__ };
	
	VkDebugUtilsMessengerCreateInfoEXT dbg_info = (VkDebugUtilsMessengerCreateInfoEXT) {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = vkDebugCallback,
		.pUserData = &bp->user_data
	};
	
	VkInstanceCreateInfo inst_info = (VkInstanceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &dbg_info,
		.flags = 0,
		.pApplicationInfo = NULL,
		.enabledExtensionCount = 3,
		.ppEnabledExtensionNames = extension_names,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = layer_names
	};

	UPDATE_DEBUG_LINE();
	VkResult res = vkCreateInstance(&inst_info, NULL, &bp->inst);
	assert(res == VK_SUCCESS);
	logt("VkInstance created\n");

	// -----------------------------------------------------------
	
	vkloadextensions(bp);

	// -----------------------------------------------------------
	
	UPDATE_DEBUG_LINE();
	res = vkCreateDebugUtilsMessengerEXTproxy(bp->inst, &dbg_info, NULL, &bp->dmessenger);
	assert(res == VK_SUCCESS);
	logt("VkDebugUtilsMessengerEXT created\n");

	// -----------------------------------------------------------

	VkWin32SurfaceCreateInfoKHR win32_info = (VkWin32SurfaceCreateInfoKHR) {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.hinstance = win->hinstance,
		.hwnd = win->hwnd
	};

	UPDATE_DEBUG_LINE();
	res = vkCreateWin32SurfaceKHR(bp->inst, &win32_info, NULL, &bp->surface);
	assert(res == VK_SUCCESS);
	logt("VkSurfaceKHR created\n");

	// -----------------------------------------------------------

  	uint32_t physical_device_count;
	UPDATE_DEBUG_LINE();
	vkEnumeratePhysicalDevices(bp->inst, &physical_device_count, NULL);
	assert(physical_device_count > 0);
	VkPhysicalDevice* phydevs = (VkPhysicalDevice*)
		malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	res = vkEnumeratePhysicalDevices(bp->inst, &physical_device_count, phydevs);
	assert(res == VK_SUCCESS);
	bp->phydev = phydevs[0];
	free(phydevs);
	logt("VkPhysicalDevice chosen\n");

	// -----------------------------------------------------------

	VkBool32 presentation_supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(bp->phydev, 0, bp->surface, &presentation_supported);
	assert(presentation_supported == VK_TRUE);
	
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_info = (VkDeviceQueueCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = 0,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};
	
	const char* device_extensions[] = { "VK_KHR_swapchain", "VK_EXT_extended_dynamic_state" };
	
	VkDeviceCreateInfo dev_info = (VkDeviceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = device_extensions,		
		.pEnabledFeatures = NULL
	};

	UPDATE_DEBUG_LINE();
	vkCreateDevice(bp->phydev, &dev_info, NULL, &bp->dev);
	logt("VkDevice created\n");

	UPDATE_DEBUG_LINE();
	vkGetDeviceQueue(bp->dev, 0, 0, &bp->queue);
	logt("VkQueue created\n");
}

void vkboilerplated(VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();
	
	UPDATE_DEBUG_LINE();
	vkDestroyDevice(bp->dev, NULL);
	logt("VkDevice destroyed\n");
	
	UPDATE_DEBUG_LINE();
	vkDestroySurfaceKHR(bp->inst, bp->surface, NULL);
	logt("VkSurfaceKHR destroyed\n");
	
	UPDATE_DEBUG_LINE();
	vkDestroyDebugUtilsMessengerEXTproxy(bp->inst, bp->dmessenger, NULL);
	logt("VkDebugUtilsMessengerEXT destroyed\n");

	UPDATE_DEBUG_LINE();
	vkDestroyInstance(bp->inst, NULL);
	logt("VkInstance destroyed\n");
}
