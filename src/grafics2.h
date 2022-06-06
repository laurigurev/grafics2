#ifndef GRAFICS2_H
#define GRAFICS2_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <vulkan.h>
#include <windows.h>

// ---------------------------------------------------------------------------------
/*
  		logger.c
 */
// ---------------------------------------------------------------------------------

typedef enum { LOG_INFO, LOG_DEBUG, LOG_TRACE, LOG_WARNING, LOG_ERROR } log_levels;

void log_init(const char* file);
void glog(int level, const char* file, int line, const char* format, ...);
void flushl();		// flush logs
void log_close();

#define logi(...) glog(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define logd(...) glog(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define logt(...) glog(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define logw(...) glog(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define loge(...) glog(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

// ---------------------------------------------------------------------------------
/*
  		fileio.c
 */
// ---------------------------------------------------------------------------------

char* file_read(const char* path, uint32_t* file_size);
void file_free(void* buffer);

// ---------------------------------------------------------------------------------
/*
  		win32.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	HINSTANCE hinstance;
	HWND hwnd;
	bool should_close;
} Window;

void windowc(Window* win, const char* name, const uint32_t width, const uint32_t height);
void windowd(Window* win);
void wpoll_events(Window* win);
void wframebuffer_size(Window* win, uint32_t* width, uint32_t* height);

// ---------------------------------------------------------------------------------
/*
  		vkdebug.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	int line;
	const char* file;
} dUserData;

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback
	(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data
	 );

// ---------------------------------------------------------------------------------
/*
  		vkboilerplate.c
 */
// ---------------------------------------------------------------------------------

PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXTproxy;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXTproxy;

typedef struct
{
	VkInstance inst;
	VkDebugUtilsMessengerEXT dmessenger;
	dUserData user_data;
	VkSurfaceKHR surface;
	VkPhysicalDevice phydev;
	VkDevice dev;
	VkQueue queue;
} VkBoilerplate;

void vkloadextensions(VkBoilerplate* bp);
void vkboilerplatec(VkBoilerplate* bp, Window* win);
void vkboilerplated(VkBoilerplate* bp);

// ---------------------------------------------------------------------------------
/*
  		vkcore.c
 */
// ---------------------------------------------------------------------------------

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct
{
	VkSwapchainKHR swapchain;
	VkExtent2D swpcext;				// swapchain extent
	uint32_t swpcimg_count;			// swapchain image count
	VkImage* swpcimgs;
	VkImageView* swpcviews;
	VkRenderPass renderpass;
	VkFramebuffer* framebuffers;
	VkCommandPool cmdpool;
	VkCommandBuffer* cmdbuffers;
	VkSemaphore* img_avb;
	VkSemaphore* render_fin;
	VkFence* in_flight;
} VkCore;

void vkcorec(VkCore* core, VkBoilerplate* bp, Window* win);
void vkcored(VkCore* core, VkBoilerplate* bp);

// ---------------------------------------------------------------------------------
/*
  		vkdoodad.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
} VkDoodad;

void vkdoodadc(VkDoodad* doodad, VkCore* core, VkBoilerplate* bp);
void vkdoodadd(VkDoodad* doodad, VkBoilerplate* bp);

// ---------------------------------------------------------------------------------
/*
  		vkapp.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkBoilerplate boilerplate;
	VkCore core;
	VkDoodad doodad;
	uint32_t current_frame;
} VkApp;

void vkappc(VkApp* app, Window* window);
void vkappd(VkApp* app);
void vkrender(VkApp* app);

#endif
