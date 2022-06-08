#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkbufferallocc(VkBufferAllocator* bufalloc, VkMemoryAllocator* memalloc,
					VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();
	
	bufalloc->page_count = 2;
	bufalloc->pages = (VkPage*) malloc(sizeof(VkPage) * bufalloc->page_count);

	VkBufferCreateInfo buffer_infos[2] = {
		(VkBufferCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.size = MEGABYTE * 2,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
					 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = NULL
		},
		(VkBufferCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.size = MEGABYTE * 2,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
					 VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = NULL
		}
	};

	VkMemoryPropertyFlags property_flags[2] = {
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	bool should_map[2] = { true, false };

	for (uint32_t i = 0; i < bufalloc->page_count; i++)
	{
		VkPage* page = bufalloc->pages + i;
		page->size = buffer_infos[i].size;

		UPDATE_DEBUG_LINE();
		VkResult res = vkCreateBuffer(bp->dev, buffer_infos + i, NULL, &page->buffer);
		assert(res == VK_SUCCESS);
		logt("VkBuffer created, page index : %i\n", i);
		flushl();

		VkBufferInfo info = (VkBufferInfo) {
			.buffer = &page->buffer,
			.size = buffer_infos[i].size,
			.property_flags = property_flags[i],
			.should_map = should_map[i],
			.mapped = NULL,
			.devmem = NULL
		};

		vkmemallocbuf(memalloc, &info, bp);

		page->devmem = info.devmem;
		page->ptr = info.mapped;

		page->suballoc_acount = 16;
		page->suballoc_count = 1;
		page->suballocs = (VkAllocation*) malloc(sizeof(VkAllocation) *
												      page->suballoc_acount);
		page->suballocs[0] = (VkAllocation) {
			.offset = 0,
			.size = buffer_infos[i].size
		};
	}
	logt("VkBufferAllocator created\n");
}

void vkbufferallocd(VkBufferAllocator* bufalloc, VkBoilerplate* bp)
{
	for (uint32_t i = 0; i < bufalloc->page_count; i++)
	{
		VkPage* page = bufalloc->pages + i;
		if (page->ptr)
		{
			vkUnmapMemory(bp->dev, *page->devmem);
			logt("buffer memory unmapped\n");
		}
		vkDestroyBuffer(bp->dev, page->buffer, NULL);
		logt("VkBuffer destroyed\n");
		page->devmem = NULL;
		page->ptr = NULL;
		free(page->suballocs);
	}
	
	free(bufalloc->pages);
	logt("VkBuffferAllocator destroyed\n");
}

void vkvbufferalloc(VkVirtualBuffer* vbuffer, VkBufferAllocator* bufalloc,
					uint32_t page_index, uint64_t size)
{
	VkPage* page = bufalloc->pages + page_index;
	for (uint32_t i = 0; i < page->suballoc_count; i++)
	{
		VkAllocation* suballoc = page->suballocs + i;
		if (size < suballoc->size)
		{
			// TODO: add correct alignements
			
			*vbuffer = (VkVirtualBuffer) {
				.page_index = page_index,
				.bufferc = page->buffer,
				.alloc_size = size,
				.size = size,
				.offset = suballoc->offset,
				.src = NULL,
				.dst = page->ptr + suballoc->offset
			};

			suballoc->offset += size;
			suballoc->size -= size;

			logt("VkVirtualBuffer allocated\n");

			return;
		}
	}

	logt("failed to allocate virtual buffer, page : %i, size : %i\n", page_index, size);
	exit(0);
}

void vkvbufferstage(VkVirtualBuffer* dst, VkBufferAllocator* bufalloc, VkBoilerplate* bp,
					VkCore* core, uint64_t size, void* src_ptr)
{
	UPDATE_DEBUG_FILE();
	
	// uint32_t src_page = 0;
	// uint32_t dst_page = 1;
	VkVirtualBuffer stage;
	vkvbufferalloc(&stage, bufalloc, 0, size);
	stage.src = src_ptr;
	memcpy(stage.dst, stage.src, size);
	vkvbufferalloc(dst, bufalloc, 1, size);

	// ----------------------------------------------------------------------------
	
	VkCommandBuffer cmdbuf_one_time;
	
	VkCommandBufferAllocateInfo cmdbuf_alloc_info = (VkCommandBufferAllocateInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = core->cmdpool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	UPDATE_DEBUG_LINE();
	VkResult res = vkAllocateCommandBuffers(bp->dev, &cmdbuf_alloc_info, &cmdbuf_one_time);
	assert(res == VK_SUCCESS);
	
	VkCommandBufferBeginInfo cmdbuf_begin_info = (VkCommandBufferBeginInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};
	UPDATE_DEBUG_LINE();
	res = vkBeginCommandBuffer(cmdbuf_one_time, &cmdbuf_begin_info);
	assert(res == VK_SUCCESS);

	// ----------------------------------------------------------------------------
	
	VkBufferCopy buffer_copy = (VkBufferCopy) {
		.srcOffset = stage.offset,
		.dstOffset = dst->offset,
		.size = stage.size
	};
	
	vkCmdCopyBuffer(cmdbuf_one_time, stage.bufferc, dst->bufferc, 1, &buffer_copy);

	// ----------------------------------------------------------------------------
	
	vkEndCommandBuffer(cmdbuf_one_time);
	
	VkSubmitInfo submit_info = (VkSubmitInfo) {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = 0,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdbuf_one_time,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};

	UPDATE_DEBUG_LINE();
	res = vkQueueSubmit(bp->queue, 1, &submit_info, VK_NULL_HANDLE);
	assert(res == VK_SUCCESS);
	
	vkQueueWaitIdle(bp->queue);
	
	vkFreeCommandBuffers(bp->dev, core->cmdpool, 1, &cmdbuf_one_time);

	// ----------------------------------------------------------------------------

	vkvbufferret(&stage, bufalloc);

	logt("VkVirtualBuffer staged\n");
}

void vkvbufferret(VkVirtualBuffer* vbuffer, VkBufferAllocator* bufalloc)
{
	VkPage* page = bufalloc->pages + vbuffer->page_index;
	for (uint32_t i = 0; i < page->suballoc_count; i++)
	{
		VkAllocation* suballoc = page->suballocs + i;
		if (vbuffer->offset + vbuffer->alloc_size == suballoc->offset)
		{
			suballoc->offset -= vbuffer->offset;
			suballoc->size += vbuffer->alloc_size;
			logt("VkVirtualBuffer returned\n");
			return;
		}
	}

	// else
	page->suballocs[page->suballoc_count] = (VkAllocation) {
		.offset = vbuffer->offset,
		.size = vbuffer->alloc_size
	};

	page->suballoc_count++;

	if (page->suballoc_count == page->suballoc_acount)
	{
		page->suballoc_acount *= 2;
		uint32_t new_size = sizeof(VkAllocation) * page->suballoc_acount;
		page->suballocs = (VkAllocation*) realloc(page->suballocs, new_size);
		logt("on page %i suballocation stack reallocated to array size %i\n",
			 vbuffer->page_index, page->suballoc_acount); 
	}
	logt("VkVirtualBuffer returned\n");
}
