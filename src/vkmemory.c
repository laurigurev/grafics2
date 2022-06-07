#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkmemallocc(VkMemoryAllocator* memalloc, VkMemoryAllocatorInfo* info, VkBoilerplate* bp)
{
	VkPhysicalDeviceMemoryProperties phydev_mem_properties;
	vkGetPhysicalDeviceMemoryProperties(bp->phydev, &phydev_mem_properties);

	memalloc->heap_count = phydev_mem_properties.memoryHeapCount;
	memalloc->heaps = (VkHeap*) malloc(sizeof(VkHeap) * memalloc->heap_count);

	for (uint32_t i = 0; i < memalloc->heap_count; i++)
	{
		VkHeap* heap = memalloc->heaps + i;
		VkMemoryHeap* mem_heap = phydev_mem_properties.memoryHeaps + i;
		
		heap->size = mem_heap->size;
	
		heap->pool_acount = 16;
		heap->pool_count = 0;
		heap->pools = (VkMemoryPool*) malloc(sizeof(VkMemoryPool) * heap->pool_acount);
	}

	for (uint32_t i = 0; i < info->pool_info_count; i++)
	{
		VkPoolInfo* pool_info = info->pool_infos + i;

		for (uint32_t k = 0; k < phydev_mem_properties.memoryTypeCount; k++)
		{
			VkMemoryType* mem_type = phydev_mem_properties.memoryTypes + k;

			if (pool_info->property_flags == mem_type->propertyFlags)
			{
				uint32_t index = mem_type->heapIndex;

				VkHeap* heap = memalloc->heaps + index;
				assert(pool_info->size < heap->size);
				heap->size -= pool_info->size;

				VkMemoryAllocateInfo alloc_info = (VkMemoryAllocateInfo) {
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   					.pNext = NULL,
					.allocationSize = pool_info->size,
					.memoryTypeIndex = k
				};
				vkmempoolc(heap->pools + heap->pool_count, &alloc_info, bp);

				heap->pool_count++;
				if (heap->pool_count == heap->pool_acount)
				{
					heap->pool_acount *= 2;
					uint64_t new_size = sizeof(VkMemoryPool) * heap->pool_acount;
					heap->pools = (VkMemoryPool*) realloc(heap->pools, new_size);
				}
				break;
			}
		}
	}
	logt("VkMemoryAllocator created\n");
}

void vkmemallocbuf(VkMemoryAllocator* memalloc, VkBufferInfo* info, VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();
	
	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(bp->dev, *info->buffer, &mem_requirements);

	VkPhysicalDeviceMemoryProperties phydev_mem_properties;
	vkGetPhysicalDeviceMemoryProperties(bp->phydev, &phydev_mem_properties);

	uint32_t heap_index;
	for (uint32_t i = 0; i < phydev_mem_properties.memoryTypeCount; i++)
	{
		if (mem_requirements.memoryTypeBits & (1 << i) &&
			phydev_mem_properties.memoryTypes[i].propertyFlags & info->property_flags)
		{
			heap_index = phydev_mem_properties.memoryTypes[i].heapIndex;
			break;
		}
	}

	bool binded = false;
	VkHeap* heap = memalloc->heaps + heap_index;
	for (uint32_t i = 0; i < heap->pool_count; i++)
	{
		VkMemoryPool* pool = heap->pools + i;
		for (uint32_t j = 0; j < pool->alloc_count; j++)
		{
			VkAllocation* alloc = pool->allocs + j;
			if (info->size < alloc->size)
			{
				// alignment stuff
				// what if new pools are needed?

				UPDATE_DEBUG_LINE();
				VkResult res = vkBindBufferMemory(
							     bp->dev, *info->buffer, pool->devmem, alloc->offset);
				assert(res == VK_SUCCESS);
				logt("buffer binded succesfully\n");
				
				binded = true;

				alloc->offset += info->size;
				alloc->size -= info->size;

				if (info->property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
					info->should_map)
				{
					UPDATE_DEBUG_LINE();
					VkResult res = vkMapMemory(
								 bp->dev, pool->devmem, 0, VK_WHOLE_SIZE, 0, &info->mapped);
					assert(res == VK_SUCCESS);
					logt("buffer mapped, pool : %i, heap : %i\n", j, i);
			    }
				// NOTE:  pool can be reallocated
				info->devmem = &pool->devmem;
				break;
			}
		}

		if (binded)
	    {
			break;
		}
	}
}

void vkmemallocd(VkMemoryAllocator* memalloc, VkBoilerplate* bp)
{
	for (uint32_t i = 0; i < memalloc->heap_count; i++)
	{
		VkHeap* heap = memalloc->heaps + i;
		for (uint32_t k = 0; k < heap->pool_count; k++)
		{
			vkmempoold(heap->pools + k, bp);
		}
		free(heap->pools);
	}
	free(memalloc->heaps);
	logt("VkMemoryAllocator destroyed\n");
}
