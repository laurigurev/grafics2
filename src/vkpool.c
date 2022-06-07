#include "grafics2.h"

#define DEFAULT_ALLOC_COUNT 16
#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkmempoolc(VkMemoryPool* pool, VkMemoryAllocateInfo* info, VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();

	pool->size = info->allocationSize;
	UPDATE_DEBUG_LINE();
	VkResult res = vkAllocateMemory(bp->dev, info, NULL, &pool->devmem);
	assert(res == VK_SUCCESS);
	logt("VkDeviceMemory allocated succesfully\n");
	
	pool->alloc_acount = DEFAULT_ALLOC_COUNT;
	pool->alloc_count = 1;
	pool->allocs = (VkAllocation*) malloc(sizeof(VkAllocation) * pool->alloc_acount);
	pool->allocs[0] = (VkAllocation) {
		.offset = 0,
		.size = pool->size
	};

	logt("VkPool created\n");
}

void vkmempoold(VkMemoryPool* pool, VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();

	UPDATE_DEBUG_LINE();
	vkFreeMemory(bp->dev, pool->devmem, NULL);
	logt("VkDeviceMemory freed\n");

	pool->alloc_acount = 0;
	pool->alloc_count = 0;
	free(pool->allocs);

	logt("VkPool destroyed\n");
}
