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
  		utils.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	char r;
	char g;
	char b;
	char a;
} pixel;

typedef long long 			i64;
typedef unsigned long long 	u64;
typedef int 				i32;
typedef unsigned int 		u32;
typedef short 				i16;
typedef unsigned short 		u16;
typedef char	 			i8;
typedef unsigned char 		u8;
typedef float				s32;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct vec2f_t
{
	s32 x;
	s32 y;
} vec2f;

typedef struct line2f_t
{
	vec2f p0;
	vec2f p1;
} line2f;

// ---------------------------------------------------------------------------------
/*
  		sort.c
 */
// ---------------------------------------------------------------------------------

// bubble sort for 'signed short'-type, aka. int16_t
void bubble_sorts(short* arr, uint64_t size);
// bubble sort for 'unsigned int'-type, aka. uint32_t
void bubble_sortui(uint32_t* arr, uint64_t size);
void bubble_sortf(s32* arr, uint64_t size);
// TODO: add faster and better sort algorithms
i32 cmp_floats_callback(const void* f0, const void* f1);

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
void arr_push(Array* arr);
void arr_remove(Array* arr, uint32_t index);
void arr_duplicates(Array* arr);
void arr_pop(Array* arr);
void arr_clean(Array* arr);
void arr_free(Array* arr);
uint64_t arr_sizeof(Array* arr);
void arr_copy(Array* arr, void* src, uint32_t count);
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
void bmp_save(const char* file, char* bmp, u32 width, u32 height, i32 bytes_per_pixel);

// ---------------------------------------------------------------------------------
/*
  		ttf.c
 */
// ---------------------------------------------------------------------------------

typedef struct TrueTypeFontTable_t
{
	u32 offset;
	u32 length;
} TrueTypeFontTable;

typedef struct TrueTypeFontGlyph_t
{
	i16 parsed;
	i16 num_contours;
	i16 x_min;
	i16 y_min;
	i16 x_max;
	i16 y_max;
	u16* end_pts_of_contours;
	u16 num_points;
	u8* flags;
	i16* pts_x;
	i16* pts_y;
	u16 aw;
	i16 lsb;
} TrueTypeFontGlyph;

typedef struct TrueTypeFontMetric_t
{
	u16 aw;
	i16 lsb;
} TrueTypeFontMetric;

typedef struct TrueTypeFont_t
{
	u16 units_per_em;
	i16 x_min;
	i16 y_min;
	i16 x_max;
	i16 y_max;
	u16 lowest_rec_ppem;
	i16 ascent;
	i16 descent;
	u16 num_hmtx;
	TrueTypeFontMetric* hmtx;
	u16 num_lsb;
	i16* lsbs;
	u16 num_glyphs;
	u16 seg_count_2;
	u16* end_code;
	u16* start_code;
	u16* id_delta;
	u16* id_range_offset;
	u32* loca;
	TrueTypeFontGlyph* glyphs;
} TrueTypeFont;

int ttf_load(TrueTypeFont** true_type_font, const char* font_path);
void ttf_free(TrueTypeFont** true_type_font);
void ttf_glyph_load(TrueTypeFont* ttf, u32 glyph_index, void* buffer,
					u64* ttf_buffer_tail_offset);
void ttf_glyph_get_hmtc(TrueTypeFont* ttf, TrueTypeFontGlyph* glyph, u32 glyph_index);
void ttf_glyph_create_deep_copy(TrueTypeFont* ttf, u32 src_index, TrueTypeFontGlyph** dst);
i32 ttf_glyph_index_get(TrueTypeFont* ttf, u16 code_point);
void* ttf_create_bitmap(TrueTypeFont* ttf, char c, u32 width, u32 height);
// TODO: user needs character specific info like aw and lsb, also bitmap width and height
void* ttf_create_font_atlas(TrueTypeFont* ttf, const char* characters, u32 point_size);

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
  		vkma_allocator.c
 */
// ---------------------------------------------------------------------------------

// TODO: comments

typedef struct VkmaSubAllocation_t
{
	u64 size;
	u64 offset;
} VkmaSubAllocation;

typedef struct VkmaAllocation_t
{
	u32 heapIndex;
	u32 blockIndex;
	VkmaSubAllocation locale;
	VkDeviceMemory memoryCopy;
	void* ptr;
	char name[64];
} VkmaAllocation;

typedef struct VkmaBlock_t
{
	u64 size;
	u64 availableSize;
	VkDeviceMemory memory;
	Array freeChunks;			// VkmaSubAllocation
	void* ptr;
} VkmaBlock;

typedef struct VkmaHeap_t
{
	Array blocks;		// VkmaBlock
} VkmaHeap;

typedef struct VkmaAllocator_t
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkPhysicalDeviceMemoryProperties phdmProps;
	VkmaHeap* heaps;
} VkmaAllocator;

typedef struct VkmaAllocatorCreateInfo_t
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
} VkmaAllocatorCreateInfo;

typedef enum VkmaAllocationUsageFlagBits_t
{
	VKMA_ALLOCATION_USAGE_AUTO = 0x01,
	VKMA_ALLOCATION_USAGE_HOST = 0x02,
	VKMA_ALLOCATION_USAGE_DEVICE = 0x04
	// VKMA_ALLOCATION_USAGE_AUTO_PREFER_HOST = 0x08,		// not implmented
	// VKMA_ALLOCATION_USAGE_AUTO_PREFER_DEVICE = 0x10		// not implmented
} VkmaAllocationUsageFlagBits;
typedef VkFlags VkmaAllocationUsageFlags;

typedef enum VkmaAllocationCreateFlagBits_t
{
	VKMA_ALLOCATION_CREATE_MAPPED = 0x01,
	VKMA_ALLOCATION_CREATE_DONT_CREATE = 0x02,
	VKMA_ALLOCATION_CREATE_DONT_BIND = 0x04,
	VKMA_ALLOCATION_CREATE_DONT_ALLOCATE = 0x08,
	VKMA_ALLOCATION_CREATE_HOST_RANDOM_ACCESS = 0x10,	// for multithreaded stuff
	VKMA_ALLOCATION_CREATE_HOST_SEQUENTIAL_WRITE = 0x20
} VkmaAllocationCreateFlagBits;
typedef VkFlags VkmaAllocationCreateFlags;

typedef struct VkmaAllocation_t VkmaAllocation;
typedef struct VkmaAllocationInfo_t
{
	VkmaAllocationUsageFlags usage;
	VkmaAllocationCreateFlags create;
	const char* name;
	void* pMappedPtr;
} VkmaAllocationInfo;

VkResult vkmaCreateAllocator(VkmaAllocator* allocator, VkmaAllocatorCreateInfo* info);
VkResult vkmaCreateBuffer(VkmaAllocator* allocator,
						  VkBufferCreateInfo* bufferInfo, VkBuffer* buffer,
						  VkmaAllocationInfo* allocInfo, VkmaAllocation* allocation);
VkResult vkmaCreateImage(VkmaAllocator* allocator,
						 VkImageCreateInfo* imageInfo, VkImage* image,
						 VkmaAllocationInfo* allocInfo, VkmaAllocation* allocation);

VkResult vkmaMapMemory(VkmaAllocator* allocator, VkmaAllocation* allocation, void** ptr);
void vkmaUnmapMemory(VkmaAllocator* allocator, VkmaAllocation* allocation);

void vkmaDestroyAllocator(VkmaAllocator* allocator);
void vkmaDestroyBuffer(VkmaAllocator* allocator, VkBuffer* buffer,
					   VkmaAllocation* allocation);
void vkmaDestroyImage(VkmaAllocator* allocator, VkImage* image, VkmaAllocation* allocation);

// ---------------------------------------------------------------------------------
/*
  		vkba_allocator.c
 */
// ---------------------------------------------------------------------------------

#define HOST_INDEX 0
#define DEVICE_INDEX 1

typedef struct VkbaPage_t
{
	VkBuffer buffer;
	VkmaAllocation allocation;
	void* ptr;
	Array freeSubAllocs;		// VmkaSubAllocation
} VkbaPage;

typedef struct VkbaAllocator_t
{
	VkbaPage* pages; 		// first is HOST, second is DEVICE
	u64 uniformBufferOffsetAlignment;
	VkDevice device;
	VkQueue queue;
	VkCommandPool commandPool;
} VkbaAllocator;

typedef struct VkbaAllocatorCreateInfo_t
{
	VkmaAllocator* allocator;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
} VkbaAllocatorCreateInfo;

typedef struct VkbaVirtualBuffer_t
{
	u32 pageIndex;
	VkBuffer buffer;
	VkmaSubAllocation locale;
	u64 range;
	void* src;
	void* dst;
} VkbaVirtualBuffer;

typedef enum VkbaVirtualBufferType_t
{
	VKBA_VIRTUAL_BUFFER_TYPE_UNIFORM = 1
	// VKBA_VIRTUAL_BUFFER_TYPE_STORAGE = 2
} VkbaVirtualBufferType;

typedef struct VkbaVirtualBufferInfo_t
{
	u32 index;
	u64 size;
	void* src;
	VkbaVirtualBufferType type;
} VkbaVirtualBufferInfo;

void vkbaCreateAllocator(VkbaAllocator* bAllocator, VkbaAllocatorCreateInfo* info);
void vkbaDestroyAllocator(VkbaAllocator* bAllocator, VkmaAllocator* allocator);
VkResult vkbaCreateVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* buffer,
								 VkbaVirtualBufferInfo* info);
VkResult vkbaStageVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* srcBuffer,
								VkbaVirtualBufferInfo* info);
void vkbaDestroyVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* buffer);

// ---------------------------------------------------------------------------------
/*
  		VULKAN DESCRIPTOR SET MANAGER
  		vkds_manager.c
 */
// ---------------------------------------------------------------------------------

typedef struct VkdsManager_t
{
	VkDevice deviceCopy;
	VkDescriptorPool descPool;
	Array descSetLayouts;		// VkDescriptorSetLayout
} VkdsManager;

typedef enum VkdsManagerCreateFlagBits_t
{
	VKDS_MANAGER_CREATE_POOL_ALLOW_FREE_BIT = 0x00000001
} VkdsManagerCreateFlagBits;
typedef VkFlags VkdsManagerCreateFlags;

typedef struct VkdsManagerCreateInfo_t
{
	VkDevice device;
	u32 poolSize;
	VkdsManagerCreateFlags flags;
} VkdsManagerCreateInfo;

typedef enum VkdsBindingType_t
{
	VKDS_BINDING_TYPE_IMAGE_SAMPLER = 1,
	VKDS_BINDING_TYPE_UNIFORM_BUFFER = 2
} VkdsBindingType;
typedef VkFlags VkdsBindingTypeFlags;

typedef enum VkdsBindingStage_t
{
	VKDS_BINDING_STAGE_VERTEX = 1,
	VKDS_BINDING_STAGE_FRAGMENT = 2
} VkdsBindingStage;
typedef VkFlags VkdsBindingStageFlags;

typedef struct VkdsBindingImageSamplerData_t
{
	VkSampler sampler;
	VkImageView view;
} VkdsBindingImageSamplerData;

typedef struct VkdsBindingUniformData_t
{
	VkbaVirtualBuffer* vbuffers;
} VkdsBindingUniformData;

typedef union VkdsBindingData_t
{
	VkdsBindingImageSamplerData imageSampler;
	VkdsBindingUniformData uniformBuffer;
} VkdsBindingData;

typedef struct VkdsBinding_t
{
	u32 location;
	VkdsBindingTypeFlags type;
	VkdsBindingStageFlags stage;
	VkdsBindingData data;
} VkdsBinding;

typedef struct VkdsDescriptorSetCreateInfo_t
{
	u32 bindingCount;
	VkdsBinding* bindings;
	u32 framesInFlight;
} VkdsDescriptorSetCreateInfo;

VkResult vkdsCreateManager(VkdsManager* manager, VkdsManagerCreateInfo* info);
VkResult vkdsCreateDescriptorSets(VkdsManager* manager, VkdsDescriptorSetCreateInfo* info,
								  VkDescriptorSet* descriptorSets,
								  VkDescriptorSetLayout* outDescSetLayout);
void vkdsDestroyManager(VkdsManager* manager);

// ---------------------------------------------------------------------------------
/*
  		vktexture.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkImage image;
	VkmaAllocation allocation;
	VkImageView view;
	VkSampler sampler;
} VkTexture;

void vktexturec(VkTexture* texture, VkBoilerplate* bp, VkCore* core,
				VkmaAllocator* mAllocator, VkbaAllocator* bAllocator);
void vktextured(VkTexture* texture, VkBoilerplate* bp, VkmaAllocator* mAllocator);
void vktransitionimglayout(VkImage* image, VkBoilerplate* bp, VkCore* core,
						  VkFormat format, VkImageLayout old_layout,
						  VkImageLayout new_layout);
void vkcopybuftoimg(VkTexture* texture, VkBoilerplate* bp, VkCore* core,
					VkbaVirtualBuffer* vBuffer, uint32_t width, uint32_t height);

// ---------------------------------------------------------------------------------
/*
  		VULKAN BINDING PIPELINE MACHINE
  		vkbp_machine.c
 */
// ---------------------------------------------------------------------------------

typedef struct VkbpMachine_t
{
	void* pool;
	u64 totalSize;
	u64 availableSize;
} VkbpMachine;

typedef enum VkbpInstruction_t
{
	VKBP_INSTRUCTION_BIND_PIPELINE = 0x00000001,
	VKBP_INSTRUCTION_BIND_VERTEX_BUFFER = 0x00000002,
	VKBP_INSTRUCTION_BIND_INDEX_BUFFER = 0x00000004,
	VKBP_INSTRUCTION_BIND_DESCRIPTOR_SETS = 0x00000008,
	VKBP_INSTRUCTION_DRAW_INDEXED = 0x00000010,
	VKBP_INSTRUCTION_START_PIPELINE = 0x00000020,
	VKBP_INSTRUCTION_END_PIPELINE = 0x00000040,
	VKBP_INSTRUCTION_BIND_INSTANCE_BUFFER = 0x00000080
} VkbpInstruction;
typedef u32 VkbpInstructionFlag;

typedef struct VkbpBindingPipelineInfo_t
{
	/*
		TODO:
		 - implementation for other draws
		 - implementation for multiple vertex buffers
		 - implementation for instancing buffer
	 */
	
	VkPipeline pipeline;
	VkbaVirtualBuffer* vertexBuffer;
	VkbaVirtualBuffer* indexBuffer;
	VkbaVirtualBuffer* instanceBuffer;
	VkPipelineLayout pipelineLayout;
	u32 maxFramesInFlight;
	u32 descriptorSetCount;
	VkDescriptorSet* descriptorSets;
	u32 indexCount;
	u32 instanceCount;
} VkbpBindingPipelineInfo;

VkResult vkbpCreateMachine(VkbpMachine* machine, u64 size);
void vkbpDestroyMachine(VkbpMachine* machine);
u64 vkbpAddBindingPipeline(VkbpMachine* machine, VkbpBindingPipelineInfo* info);
VkResult vkbpBindBindingPipeline(VkbpMachine* machine, VkCommandBuffer cmd, u32 frame,
								  u64 offset);

// ---------------------------------------------------------------------------------
/*
  		VULKAN ENHANCED PIPELINE
  		vken_pipeline.c
 */
// ---------------------------------------------------------------------------------

typedef struct VkenPipeline_t
{
	VkDevice device_copy;
	VkPipeline pipe;
	VkPipelineLayout layout;
} VkenPipeline;

typedef struct VkenPipelineCreateInfo_t
{
	char* vertex_shader_path;
	char* fragment_shader_path;
	u32 vertex_input_binding_description_count;
	VkVertexInputBindingDescription* vertex_input_binding_descriptions;
	u32 vertex_input_attribute_description_count;
	VkVertexInputAttributeDescription* vertex_input_attribute_descriptions;
	u32 descriptor_set_layout_count;
	VkDescriptorSetLayout* layouts;
	VkExtent2D extent;
	VkDevice device;
	VkRenderPass renderpass;
	u32 subpass_index;
} VkenPipelineCreateInfo;

typedef struct VkenPipelineCache_t VkenPipelineCache;

// void vkenInitializeCache();
VkResult vkenCreatePipelines(u32 pipeline_count, VkenPipeline* pipelines,
							 VkenPipelineCreateInfo* infos);
void vkenDestroyPipeline(VkenPipeline* pipeline);

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
	u64 bindingId;
	VkenPipeline pipeline;
	VkbaVirtualBuffer vertexbuff;
	VkbaVirtualBuffer indexbuff;
	VkbaVirtualBuffer instbuff;
	
	VkTexture texture;
	float uboData[3];
	VkbaVirtualBuffer ubos[MAX_FRAMES_IN_FLIGHT];

	VkDescriptorSetLayout dlayout;
	VkDescriptorSet dsets[MAX_FRAMES_IN_FLIGHT];
} VkDoodad;

void vkdoodadc(VkDoodad* doodad, VkbaAllocator* bAllocator, VkmaAllocator* mAllocator, 
			   VkCore* core, VkBoilerplate* bp, VkdsManager* dsManager);
void vkdoodadd(VkDoodad* doodad, VkbaAllocator* bAllocator,
			   VkBoilerplate* bp, VkmaAllocator* mAllocator);

// ---------------------------------------------------------------------------------
/*
  		vkapp.c
 */
// ---------------------------------------------------------------------------------

typedef struct
{
	VkBoilerplate boilerplate;
	VkCore core;
	VkmaAllocator memory_allocator;
	VkbaAllocator buffer_allocator;
	VkdsManager dsManager;
	VkbpMachine machine;
	VkDoodad doodad;
	uint32_t current_frame;
} VkApp;

void vkappc(VkApp* app, Window* window);
void vkappd(VkApp* app);
void vkrender(VkApp* app);

#endif
