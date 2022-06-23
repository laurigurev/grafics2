#include "grafics2.h"

#define vkba_logi(...) logi(__VA_ARGS__)
#define vkba_logd(...) logd(__VA_ARGS__)
#define vkba_logw(...) logw(__VA_ARGS__)
#define vkba_loge(...) loge(__VA_ARGS__)

const char* location_names[] = {
	"HOST", "DEVICE"
};

void vkbaCreateAllocator(VkbaAllocator* bAllocator, VkbaAllocatorCreateInfo* info)
{
	assert(bAllocator != NULL);
	assert(info != NULL);
	VkResult result;

	bAllocator->pages = (VkbaPage*) malloc(sizeof(VkbaPage) * 2);

	// create buffer on host
	VkBufferCreateInfo bufferInfo = (VkBufferCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = MEGABYTE,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
				 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
				 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL
	};

	VkmaAllocationInfo allocInfo = (VkmaAllocationInfo) {
		.usage = VKMA_ALLOCATION_USAGE_HOST,
		.create = VKMA_ALLOCATION_CREATE_MAPPED |
				  VKMA_ALLOCATION_CREATE_HOST_SEQUENTIAL_WRITE,
		.name = location_names[HOST_INDEX],
		.pMappedPtr = NULL,
	};
	
	result = vkmaCreateBuffer(info->allocator,
							  &bufferInfo, &bAllocator->pages[HOST_INDEX].buffer,
							  &allocInfo, &bAllocator->pages[HOST_INDEX].allocation);
	assert(result == VK_SUCCESS);

	bAllocator->pages[HOST_INDEX].ptr = allocInfo.pMappedPtr;
	arr_init(&bAllocator->pages[HOST_INDEX].freeSubAllocs, sizeof(VkmaSubAllocation));
	// arr_add(&bAllocator->pages[HOST_INDEX].freeSubAllocs, &allocInfo.subAlloc);
	arr_add(&bAllocator->pages[HOST_INDEX].freeSubAllocs,
			&bAllocator->pages[HOST_INDEX].allocation.locale);

	// create buffer on device
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		               VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	allocInfo.usage = VKMA_ALLOCATION_USAGE_DEVICE;
	allocInfo.create = 0;
	allocInfo.name = location_names[DEVICE_INDEX],
	allocInfo.pMappedPtr = NULL;

	result = vkmaCreateBuffer(info->allocator,
							  &bufferInfo, &bAllocator->pages[DEVICE_INDEX].buffer,
							  &allocInfo, &bAllocator->pages[DEVICE_INDEX].allocation);
	assert(result == VK_SUCCESS);

	arr_init(&bAllocator->pages[DEVICE_INDEX].freeSubAllocs, sizeof(VkmaSubAllocation));
	// arr_add(&bAllocator->pages[DEVICE_INDEX].freeSubAllocs, &allocInfo.subAlloc);
	arr_add(&bAllocator->pages[DEVICE_INDEX].freeSubAllocs,
			&bAllocator->pages[DEVICE_INDEX].allocation.locale);

	bAllocator->device = info->device;
	bAllocator->queue = info->queue;

	VkCommandPoolCreateInfo cmdPoolInfo = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL,
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 0
	};
	result = vkCreateCommandPool(bAllocator->device, &cmdPoolInfo,
								 NULL, &bAllocator->commandPool);
	assert(result == VK_SUCCESS);

	vkba_logi("[vkba] Buffer Allocator created\n");
}

void vkbaDestroyAllocator(VkbaAllocator* bAllocator, VkmaAllocator* allocator)
{
	vkDestroyCommandPool(bAllocator->device, bAllocator->commandPool, NULL);
	bAllocator->queue = VK_NULL_HANDLE;
	bAllocator->device = VK_NULL_HANDLE;
	
	VkbaPage* page = bAllocator->pages + HOST_INDEX;
	vkmaDestroyBuffer(allocator, &page->buffer, &page->allocation);
	page->ptr = NULL;
	arr_free(&page->freeSubAllocs);

	page = bAllocator->pages + DEVICE_INDEX;
	vkmaDestroyBuffer(allocator, &page->buffer, &page->allocation);
	page->ptr = NULL;
	arr_free(&page->freeSubAllocs);

	free(bAllocator->pages);

	vkba_logi("[vkba] Buffer allocator destroyed\n");
}

VkResult vkbaCreateVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* buffer,
							 VkbaVirtualBufferInfo* info)
{
	VkbaPage* page = bAllocator->pages + info->index;
	VkmaSubAllocation* freeSubAlloc = (VkmaSubAllocation*) arr_get(&page->freeSubAllocs, 0);
	for (u32 i = 0; i < page->freeSubAllocs.size; i++) {
		if (info->size <= freeSubAlloc->size) {
			buffer->pageIndex = info->index;
			buffer->buffer = page->buffer;
			buffer->locale = (VkmaSubAllocation) { info->size, freeSubAlloc->offset };

			freeSubAlloc->size -= info->size;
			freeSubAlloc->offset += info->size;

			buffer->src = info->src;
			buffer->dst = page->ptr + buffer->locale.offset;
			
			return VK_SUCCESS;
		}
		freeSubAlloc++;
	}
	vkba_loge("[vkba] Failed to create virtual buffer on '%s' buffer\n",
			  location_names[info->index]);
	return VK_ERROR_UNKNOWN;
}

VkResult vkbaStageVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* srcBuffer,
								VkbaVirtualBufferInfo* info)
{
	VkResult result;
	
	VkbaVirtualBuffer stagingBuffer;
	VkbaVirtualBufferInfo tmpBufferInfo = { HOST_INDEX, info->size, info->src };
	vkbaCreateVirtualBuffer(bAllocator, &stagingBuffer, &tmpBufferInfo);
	memcpy(stagingBuffer.dst, stagingBuffer.src, stagingBuffer.locale.size);
	tmpBufferInfo = (VkbaVirtualBufferInfo) {
		info->index /* DEVICE_INDEX */, info->size, stagingBuffer.dst
	};
	vkbaCreateVirtualBuffer(bAllocator, srcBuffer, &tmpBufferInfo);

	// ----------------------------------------------------------------------------

	VkCommandBuffer cmdBuffer;
	VkCommandBufferAllocateInfo cmdBufferAllocInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, bAllocator->commandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1
	};
	result = vkAllocateCommandBuffers(bAllocator->device, &cmdBufferAllocInfo, &cmdBuffer);
	assert(result == VK_SUCCESS);

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL
	};
	result = vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
	assert(result == VK_SUCCESS);

	// ----------------------------------------------------------------------------

	VkBufferCopy bufferCopy = {
		srcBuffer->locale.offset, stagingBuffer.locale.offset, info->size
	};
	vkCmdCopyBuffer(cmdBuffer, stagingBuffer.buffer, srcBuffer->buffer, 1, &bufferCopy);

	// ----------------------------------------------------------------------------

	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmdBuffer, 0, NULL
	};
	result = vkQueueSubmit(bAllocator->queue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(result == VK_SUCCESS);

	vkQueueWaitIdle(bAllocator->queue);
	vkFreeCommandBuffers(bAllocator->device, bAllocator->commandPool, 1, &cmdBuffer);

	vkbaDestroyVirtualBuffer(bAllocator, &stagingBuffer);

	// ----------------------------------------------------------------------------

	vkba_logi("[vkba] Virtual buffer staged succesfully\n");
	
	return VK_SUCCESS;
}

void vkbaDestroyVirtualBuffer(VkbaAllocator* bAllocator, VkbaVirtualBuffer* buffer)
{
	VkbaPage* page = bAllocator->pages + buffer->pageIndex;
	VkmaSubAllocation* freeSubAlloc = (VkmaSubAllocation*) arr_get(&page->freeSubAllocs, 0);
	VkmaSubAllocation locale = buffer->locale;
	for (u32 i = 0; i < page->freeSubAllocs.size; i++) {
		if (locale.offset + locale.size == freeSubAlloc->offset) {
			freeSubAlloc->size += locale.size;
			freeSubAlloc->offset -= locale.size;

			buffer->pageIndex = 0;
			buffer->buffer = VK_NULL_HANDLE;
			buffer->locale = (VkmaSubAllocation) { 0, 0 };

			buffer->src = NULL;
			buffer->dst = NULL;
			
			return;
		}
		freeSubAlloc++;
	}
	vkba_loge("[vkba] Failed to destroy virtual buffer in '%s'\n",
			  location_names[buffer->pageIndex]);
}
