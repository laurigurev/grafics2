#ifndef GRAFICS2_H
#define GRAFICS2_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <vulkan.h>
#include <windows.h>

#define KILOBYTE 1024
#define MEGABYTE 1024*1024
#define GIGABYTE 1024*1024*1024

// ---------------------------------------------------------------------------------
/*
  		sort.c
 */
// ---------------------------------------------------------------------------------

// bubble sort for 'signed short'-type, aka. int16_t
void bubble_sorts(short* arr, uint64_t size);
// TODO: add faster and better sort algorithms

// ---------------------------------------------------------------------------------
/*
  		array.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	uint32_t		stride;
	uint32_t 		alloc_size;
	uint32_t 		size;
	void* 			data;
} Array;

void arr_init(Array* arr, uint64_t stride);
void arr_add(Array* arr, void* src);
void arr_insert();								// TODO
void arr_insertm();								// TODO: insert multiple
void arr_push();								// TODO
void arr_rem();									// TODO
void arr_pop(Array* arr);
void arr_clean(Array* arr);
void arr_free(Array* arr);
uint64_t arr_sizeof(Array* arr);
void arr_copy(Array* arr, uint32_t count, void* src); // TODO
void* arr_get(Array* arr, uint32_t index);
// void arr_sort(Array* arr);

// ---------------------------------------------------------------------------------
/*
  		memory.c
 */
// ---------------------------------------------------------------------------------

// variadic malloc
// vmalloc(size, &ptr, size, &ptr);
// void vmalloc(...);		// TODO

// variadic free
// vfree(arr1, arr2, arr3, ...);
// void vfree(...);		// TODO

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
  		bmploader.c
 */
// ---------------------------------------------------------------------------------

char* bmp_load(const char* file, uint32_t* width, uint32_t* height);
void bmp_free(char* buffer);

// ---------------------------------------------------------------------------------
/*
  		ttfparser.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	uint32_t scaler_type;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
} ttf_table_directory;

typedef struct
{
	uint32_t tag;
	uint32_t checkSum;
	uint32_t offset;
	uint32_t length;
} ttf_header;

typedef struct
{
	uint32_t version;
	uint32_t font_revision;
	uint32_t checksum_adjustment;
	uint32_t magic_number;
	uint16_t flags;
	uint16_t units_per_em;
	int64_t created;
	int64_t modified;
	int16_t x_min;
	int16_t y_min;
	int16_t x_max;
	int16_t y_max;
	uint16_t mac_style;
	uint16_t lowest_rec_ppem;
	int16_t font_direction_hint;
	int16_t index_to_loc_format;
	int16_t glyph_data_format;
} ttf_head;

typedef struct
{
	uint32_t version;
	int16_t ascent;
	int16_t descent;
	int16_t line_gap;
	uint16_t advance_with_max;
	int16_t min_left_side_bearing;
	int16_t min_right_side_bearing;
	int16_t x_max_extent;
	int16_t caret_slope_rise;
	int16_t caret_slope_run;
	int16_t caret_offset;
	int16_t reserved[4];
	int16_t metric_date_format;
	uint16_t num_of_long_hor_metrics;
} ttf_hhea;

typedef struct
{
	uint32_t version;
	uint16_t num_glyphs;
	uint16_t max_points;
	uint16_t max_contours;
	uint16_t max_component_points;
	uint16_t max_component_contours;
	uint16_t max_zones;
	uint16_t max_twilight_points;
	uint16_t max_storage;
	uint16_t max_function_defs;
	uint16_t max_instruction_defs;
	uint16_t max_stack_elements;
	uint16_t max_size_of_instructions;
	uint16_t max_component_elements;
	uint16_t max_component_depth;
} ttf_maxp;

typedef struct
{
	uint16_t advance_width;
	int16_t left_side_bearing;
} ttf_long_hor_metric;

typedef struct
{
	ttf_long_hor_metric* h_metrics;
	int16_t* left_side_bearing;
} ttf_hmtx;

typedef struct
{
	uint16_t version;
	uint16_t number_subtables;
} ttf_cmap_index;

typedef struct
{
	uint16_t platform_id;
	uint16_t platform_specific_id;
	uint32_t offset;
} ttf_cmap_estable;

typedef struct
{
	uint16_t format;
	uint16_t length;
	uint16_t language;
	uint16_t seg_count_2;
	uint16_t search_range;
	uint16_t entry_selector;
	uint16_t range_shift;
	uint16_t* end_code;
	uint16_t reserved_pad;
	uint16_t* start_code;
	uint16_t* id_delta;
	uint16_t* id_range_offset;
	uint16_t* glyph_index_array;
} ttf_cmap_format4;

typedef struct
{
	ttf_cmap_index index;
	ttf_cmap_estable* tables;
	ttf_cmap_format4 format;
} ttf_cmap;

typedef struct
{
	int16_t num_of_contours;
	int16_t	x_min;
	int16_t	y_min;
	int16_t	x_max;
	int16_t	y_max;
} ttf_glyf_header;

typedef struct
{
	// uint16_t num_contours;
	uint16_t* end_contours;
	uint16_t instruction_len;
	uint8_t* instructions;
	uint16_t num_points;
	uint8_t* flags;
	int16_t* px;
	int16_t* py;
} ttf_glyf_data;

typedef struct
{
	ttf_glyf_header header;
	ttf_glyf_data data;
} ttf_glyf;

typedef struct
{	
	ttf_head* head;
	ttf_hhea* hhea;
	ttf_maxp* maxp;
	ttf_hmtx* hmtx;
	ttf_cmap* cmap;
	uint32_t* loca;
	ttf_glyf* glyf;
} ttf_core;

// cmap, glyf, head, hhea, hmtx, loca, maxp are MANDATORY
// everything else is just additions

void ttf_load(ttf_core* ttf);
void ttf_free(ttf_core* ttf);

void ttf_head_load(ttf_core* ttf, void* file);
void ttf_hhea_load(ttf_core* ttf, void* file);
void ttf_maxp_load(ttf_core* ttf, void* file);
void ttf_hmtx_load(ttf_core* ttf, void* file);
void ttf_cmap_load(ttf_core* ttf, void* file);
void ttf_loca_load(ttf_core* ttf, void* file);
void ttf_glyf_load(ttf_core* ttf, void* file); // TODO

void ttf_glyf_data_load(ttf_core* ttf, ttf_glyf* glyf, void* file, uint32_t offset);
void ttf_cmap_format4_table_load(ttf_cmap* cmap, void* file);

void ttf_hmtx_free(ttf_core* ttf);
void ttf_cmap_free(ttf_core* ttf);
void ttf_loca_free(ttf_core* ttf);
void ttf_glyf_data_free(ttf_glyf* glyf);
void ttf_glyf_free(ttf_core* ttf);

void* ttf_to_bmp(uint32_t width, uint32_t height, ttf_core* ttf);

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
PFN_vkCmdBindVertexBuffers2EXT vkCmdBindVertexBuffers2EXTproxy;

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
  		vkpool.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	uint64_t offset;
	uint64_t size;
} VkAllocation;

typedef struct
{
	uint64_t size;
	VkDeviceMemory devmem;

	uint32_t alloc_acount;
	uint32_t alloc_count;
	VkAllocation* allocs;
} VkMemoryPool;

void vkmempoolc(VkMemoryPool* pool, VkMemoryAllocateInfo* info, VkBoilerplate* bp);
void vkmempoold(VkMemoryPool* pool, VkBoilerplate* bp);

// ---------------------------------------------------------------------------------
/*
  		vkmemory.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	uint64_t size;

	uint32_t pool_acount;
	uint32_t pool_count;
	VkMemoryPool* pools;
} VkHeap;

typedef struct
{
	uint32_t heap_count;
	VkHeap* heaps;
} VkMemoryAllocator;

typedef struct
{
	VkMemoryPropertyFlags property_flags;
	uint64_t size;
} VkPoolInfo;

typedef struct
{
	uint32_t pool_info_count;
	VkPoolInfo* pool_infos;
} VkMemoryAllocatorInfo;

typedef struct
{
	VkBuffer* buffer;		// has to be a valid buffer
	uint64_t size;
	VkMemoryPropertyFlags property_flags;
	bool should_map;
	void* mapped;
	VkDeviceMemory* devmem;
} VkBufferInfo;

void vkmemallocc(VkMemoryAllocator* memalloc, VkMemoryAllocatorInfo* info, VkBoilerplate* bp);
void vkmemallocbuf(VkMemoryAllocator* memalloc, VkBufferInfo* info, VkBoilerplate* bp);
void vkmemallocimg(VkMemoryAllocator* memalloc, VkBoilerplate* bp, VkImage* image);
void vkmemallocd(VkMemoryAllocator* memalloc, VkBoilerplate* bp);

// ---------------------------------------------------------------------------------
/*
  		vkbufferalloc.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	uint64_t size;
	VkBuffer buffer;
	VkDeviceMemory* devmem;

	void* ptr;
	
	uint32_t suballoc_acount;
	uint32_t suballoc_count;
	VkAllocation* suballocs;
} VkPage;

typedef struct
{
	uint32_t page_count;
	VkPage* pages;
} VkBufferAllocator;

void vkbufferallocc(
		VkBufferAllocator* bufalloc, VkMemoryAllocator* memalloc, VkBoilerplate* bp);
void vkbufferallocd(VkBufferAllocator* bufalloc, VkBoilerplate* bp);

typedef struct
{
	uint32_t page_index;
	VkBuffer bufferc;
	uint64_t alloc_size;
	uint64_t size;
	uint64_t offset;
	void* src;
	void* dst;
} VkVirtualBuffer;

void vkvbufferalloc(VkVirtualBuffer* vbuffer, VkBufferAllocator* bufalloc,
					uint32_t page_index, uint64_t size);
void vkvbufferstage(VkVirtualBuffer* dst, VkBufferAllocator* bufalloc, VkBoilerplate* bp,
					VkCore* core, uint64_t size, void* src_ptr);
void vkvbufferret(VkVirtualBuffer* vbuffer, VkBufferAllocator* bufalloc);

// ---------------------------------------------------------------------------------
/*
  		vktexture.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkImage image;
	VkImageView view;
	VkSampler sampler;
} VkTexture;

void vktexturec(VkTexture* texture, VkBoilerplate* bp, VkCore* core,
				VkMemoryAllocator* memalloc, VkBufferAllocator* bufalloc);
void vktextured(VkTexture* texture, VkBoilerplate* bp);
void vktransitionimglayout(VkImage* image, VkBoilerplate* bp, VkCore* core,
						  VkFormat format, VkImageLayout old_layout,
						  VkImageLayout new_layout);
void vkcopybuftoimg(VkTexture* texture, VkBoilerplate* bp, VkCore* core,
					VkVirtualBuffer* vbuf, uint32_t width, uint32_t height);

// ---------------------------------------------------------------------------------
/*
  		vkdoodad.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	float position[2];
	float uv[2];
} Vertex;

typedef struct
{
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
	VkVirtualBuffer vertexbuff;
	VkVirtualBuffer indexbuff;
	VkTexture texture;

	VkDescriptorSetLayout dlayout;
	VkDescriptorSet dsets[MAX_FRAMES_IN_FLIGHT];
} VkDoodad;

void vkdoodadc(VkDoodad* doodad, VkBufferAllocator* bufalloc, VkMemoryAllocator* memalloc, 
			   VkCore* core, VkBoilerplate* bp, VkDescriptorPool* dpool);
void vkdoodadd(VkDoodad* doodad, VkBufferAllocator* bufalloc, VkBoilerplate* bp);
void vkdoodadb(VkDoodad* doodad, VkCommandBuffer cmdbuf, uint32_t current_frame);

// ---------------------------------------------------------------------------------
/*
  		vkapp.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkBoilerplate boilerplate;
	VkCore core;
	VkMemoryAllocator memalloc;
	VkBufferAllocator buffalloc;
	VkDescriptorPool dpool;
	VkDoodad doodad;
	uint32_t current_frame;
} VkApp;

void vkappc(VkApp* app, Window* window);
void vkappd(VkApp* app);
void vkrender(VkApp* app);

#endif
