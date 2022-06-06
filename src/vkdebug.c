#include "grafics2.h"

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback
	(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data
	)
{
	dUserData* debug_user_data = (dUserData*) user_data;

	if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		glog(
			LOG_INFO, debug_user_data->file, debug_user_data->line,
			"[validation layers] %s\n", callback_data->pMessage
		);
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		glog(
			LOG_INFO, debug_user_data->file, debug_user_data->line,
			"[validation layers] %s\n", callback_data->pMessage
		);
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		glog(
			LOG_WARNING, debug_user_data->file, debug_user_data->line,
			"[validation layers] %s\n", callback_data->pMessage
		);
	}
	else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		glog(
			LOG_ERROR, debug_user_data->file, debug_user_data->line,
			"[validation layers] %s\n", callback_data->pMessage
		);
	}

	return VK_FALSE;
}
