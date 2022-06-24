#include "grafics2.h"

#define VKMA_DEFAULT_MEMORY_BLOCK_SIZE 256 * MEGABYTE

#define vkma_logi(...) logi(__VA_ARGS__)
#define vkma_logd(...) logd(__VA_ARGS__)
#define vkma_logw(...) logw(__VA_ARGS__)
#define vkma_loge(...) loge(__VA_ARGS__)

typedef struct VkmaAllocator_t 		VkmaAllocator;
typedef struct VkmaHeap_t 			VkmaHeap;
typedef struct VkmaBlock_t 			VkmaBlock;
typedef struct VkmaAllocation_t 	VkmaAllocation;

VkResult vkmaCreateAllocator(VkmaAllocator* allocator, VkmaAllocatorCreateInfo* info)
{
	allocator->physicalDevice = info->physicalDevice;
	allocator->device = info->device;
	vkGetPhysicalDeviceMemoryProperties(allocator->physicalDevice, &allocator->phdmProps);
	allocator->heaps = (VkmaHeap*) malloc(sizeof(VkmaHeap) *
										  allocator->phdmProps.memoryHeapCount);

	for (u32 i = 0; i < allocator->phdmProps.memoryHeapCount; i++) {
		VkmaHeap* heap = allocator->heaps + i;
		arr_init(&heap->blocks, sizeof(VkmaBlock));
	}
	
	return VK_SUCCESS;
}

VkResult vkmaCreateBuffer(VkmaAllocator* allocator,
						  VkBufferCreateInfo* bufferInfo, VkBuffer* buffer,
						  VkmaAllocationInfo* allocInfo, VkmaAllocation* allocation)
{
	assert(allocator != NULL);
	assert(buffer != NULL);
	assert(allocInfo != NULL);
	assert(allocation != NULL);

	VkResult result;
	
	if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_CREATE)) {
		result = vkCreateBuffer(allocator->device, bufferInfo, NULL, buffer);
		assert(result == VK_SUCCESS);
	}
	
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(allocator->device, *buffer, &memReqs);

	// generate property flag
	VkMemoryPropertyFlags propertyFlagsLUTs[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	VkMemoryPropertyFlags PROPERTY_FLAGS = 0;
	
	if (allocInfo->usage & VKMA_ALLOCATION_USAGE_AUTO) {
		// default is always going to be local device,
		// but depending on other mapping options
		// it maybe be changed to some host variant
		PROPERTY_FLAGS = propertyFlagsLUTs[0];
		if (allocInfo->create & VKMA_ALLOCATION_CREATE_MAPPED) {
			// no need to copy same code again
			goto USAGE_HOST;
		}
	} else if (allocInfo->usage & VKMA_ALLOCATION_USAGE_HOST) {
		USAGE_HOST:
		
		PROPERTY_FLAGS = propertyFlagsLUTs[1];
		if (allocInfo->create & VKMA_ALLOCATION_CREATE_HOST_RANDOM_ACCESS) {
			PROPERTY_FLAGS = propertyFlagsLUTs[2];
			vkma_logw("[vkma] VKMA_ALLOCATION_CREATE_HOST_RANDOM_ACCESS not supported\n");
		} else if (allocInfo->create & VKMA_ALLOCATION_CREATE_HOST_SEQUENTIAL_WRITE) {
			PROPERTY_FLAGS = propertyFlagsLUTs[3];
		}
	} else if (allocInfo->usage & VKMA_ALLOCATION_USAGE_DEVICE) {
		PROPERTY_FLAGS = propertyFlagsLUTs[0];
	}

	if (PROPERTY_FLAGS == 0) {
		vkma_logw("[vkma] Current usage flags unsupported\n");
		return VK_ERROR_UNKNOWN;
	}

	i64 heapIndex = -1;
	i64 memoryTypeIndex = -1;
	for (u32 i = 0; i < allocator->phdmProps.memoryTypeCount; i++) {
		if (memReqs.memoryTypeBits & (1 << i) &&
			(allocator->phdmProps.memoryTypes[i].propertyFlags & PROPERTY_FLAGS) ==
			PROPERTY_FLAGS) {
			memoryTypeIndex = i;
			heapIndex = allocator->phdmProps.memoryTypes[i].heapIndex;
			break;
		}
	}

	if (heapIndex == -1) {
		vkma_loge("[vkma] failed to find an appropriate heap\n");
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// bind buffer

	VkmaHeap* heap = allocator->heaps + heapIndex;
	for (u32 i = 0; i < heap->blocks.size; i++) {
		VkmaBlock* block = arr_get(&heap->blocks, i);
		if (bufferInfo->size < block->availableSize) {
			for (u32 j = 0; j <= block->freeChunks.size; j++) {
				VkmaSubAllocation* chunk = arr_get(&block->freeChunks, j);
				if (bufferInfo->size <= chunk->size) {
					// bind buffer
					if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_BIND)) {
						result = vkBindBufferMemory(allocator->device, *buffer,
													block->memory, chunk->offset);
						assert(result == VK_SUCCESS);
					}

					// create allocation
					allocation->heapIndex = heapIndex;
					allocation->blockIndex = i;
					allocation->locale = (VkmaSubAllocation) {
						bufferInfo->size, chunk->offset
					};
					allocation->memoryCopy = block->memory;
					allocation->ptr = NULL;
					memset(allocation->name, 0, 64);
					u64 nameLen = strlen(allocInfo->name);
					if (64 <= nameLen) { nameLen = 63; }
					memcpy(allocation->name, allocInfo->name, nameLen);

					// update a chunk
					block->availableSize -= bufferInfo->size;
					chunk->size -= bufferInfo->size;
					chunk->offset += bufferInfo->size;

					// map buffer if necessary
					if (allocInfo->create & VKMA_ALLOCATION_CREATE_MAPPED) {
						result = vkmaMapMemory(allocator, allocation, &allocInfo->pMappedPtr);
						assert(result == VK_SUCCESS);
					}

					vkma_logi("[vkma] Buffer Allocation '%s' created: heapIndex %hu, "
							  "blockIndex %hu, size %lu, offset %lu\n",
							  allocation->name, allocation->heapIndex, allocation->blockIndex,
							  allocation->locale.size, allocation->locale.offset);
					
					return VK_SUCCESS;
				}
			}
		}
	}

	if (allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_ALLOCATE) {
		vkma_loge("[vkma] failed to find big enough existing memory block\n");
		return VK_ERROR_UNKNOWN;
	}

	// we will have to create a new memory block
	u64 blockSize = VKMA_DEFAULT_MEMORY_BLOCK_SIZE;
	if (blockSize < bufferInfo->size) {
		// make sure blockSize is going to be a multiple of 256 MB
		blockSize = bufferInfo->size;
		blockSize += (blockSize % VKMA_DEFAULT_MEMORY_BLOCK_SIZE);
	}
	VkMemoryAllocateInfo memAllocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, blockSize, memoryTypeIndex
	};

	arr_push(&heap->blocks);
	VkmaBlock* tmpBlock = (VkmaBlock*) arr_get(&heap->blocks, heap->blocks.size - 1);
	tmpBlock->size = blockSize;
	tmpBlock->availableSize = blockSize;
	tmpBlock->ptr = NULL;

	result = vkAllocateMemory(allocator->device, &memAllocInfo, NULL, &tmpBlock->memory);
	assert(result == VK_SUCCESS);

	arr_init(&tmpBlock->freeChunks, sizeof(VkmaSubAllocation));
	VkmaSubAllocation subAlloc = { tmpBlock->availableSize, 0 };
		
	// bind buffer
	if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_BIND)) {
		result = vkBindBufferMemory(allocator->device, *buffer, tmpBlock->memory, 0);
		assert(result == VK_SUCCESS);
	}

	// update allocation and freeChunks
	tmpBlock->availableSize -= bufferInfo->size;
	subAlloc.size -= bufferInfo->size;
	subAlloc.offset += bufferInfo->size;

	arr_add(&tmpBlock->freeChunks, &subAlloc);

	// create allocation
	allocation->heapIndex = heapIndex;
	allocation->blockIndex = heap->blocks.size - 1;
	allocation->locale = (VkmaSubAllocation) { bufferInfo->size, 0 };
	allocation->memoryCopy = tmpBlock->memory;
	allocation->ptr = NULL;
	// memset(allocation->name, 0, sizeof(allocation->name));	// SEGFAULT
	u64 ii = sizeof(allocation->name);
	while(ii--) {
		allocation->name[ii] = 0;
	}
	u64 nameLen = strlen(allocInfo->name);
	if (64 <= nameLen) { nameLen = 63; }
	memcpy(allocation->name, allocInfo->name, nameLen);
	// strncpy(allocation->name, allocInfo->name, sizeof(allocation->name));

	// map buffer if necessary
	if (allocInfo->create & VKMA_ALLOCATION_CREATE_MAPPED) {
		result = vkmaMapMemory(allocator, allocation, &allocInfo->pMappedPtr);
		assert(result == VK_SUCCESS);
	}

	vkma_logi("[vkma] Buffer Allocation '%s' created: heapIndex %hu, "
			  "blockIndex %hu, size %lu, offset %lu\n",
			  allocation->name, allocation->heapIndex, allocation->blockIndex,
			  allocation->locale.size, allocation->locale.offset);
	
	return VK_SUCCESS;
}

VkResult vkmaCreateImage(VkmaAllocator* allocator,
						 VkImageCreateInfo* imageInfo, VkImage* image,
						 VkmaAllocationInfo* allocInfo, VkmaAllocation* allocation)
{
	assert(allocator != NULL);
	assert(image != NULL);
	assert(allocInfo != NULL);
	assert(allocation != NULL);

	VkResult result;
	
	if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_CREATE)) {
		result = vkCreateImage(allocator->device, imageInfo, NULL, image);
		assert(result == VK_SUCCESS);
	}
	
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(allocator->device, *image, &memReqs);

	// generate property flag
	VkMemoryPropertyFlags propertyFlagsLUTs[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	VkMemoryPropertyFlags PROPERTY_FLAGS = 0;

	if (allocInfo->usage & VKMA_ALLOCATION_USAGE_AUTO) {
		// default is always going to be local device,
		// but depending on other mapping options
		// it maybe be changed to some host variant
		PROPERTY_FLAGS = propertyFlagsLUTs[0];
		if (allocInfo->create & VKMA_ALLOCATION_CREATE_MAPPED) {
			// no need to copy same code again
			goto USAGE_HOST;
		}
	} else if (allocInfo->usage & VKMA_ALLOCATION_USAGE_HOST) {
		USAGE_HOST:
		
		PROPERTY_FLAGS = propertyFlagsLUTs[1];
		if (allocInfo->create & VKMA_ALLOCATION_CREATE_HOST_RANDOM_ACCESS) {
			PROPERTY_FLAGS = propertyFlagsLUTs[2];
			vkma_logw("[vkma] VKMA_ALLOCATION_CREATE_HOST_RANDOM_ACCESS not supported\n");
		} else if (allocInfo->create & VKMA_ALLOCATION_CREATE_HOST_SEQUENTIAL_WRITE) {
			PROPERTY_FLAGS = propertyFlagsLUTs[3];
		}
	} else if (allocInfo->usage & VKMA_ALLOCATION_USAGE_DEVICE) {
		PROPERTY_FLAGS = propertyFlagsLUTs[0];
	}

	if (PROPERTY_FLAGS == 0) {
		vkma_logw("[vkma] Current usage flags unsupported\n");
		return VK_ERROR_UNKNOWN;
	}

	i64 heapIndex = -1;
	i64 memoryTypeIndex = -1;
	for (u32 i = 0; i < allocator->phdmProps.memoryTypeCount; i++) {
		if (memReqs.memoryTypeBits & (1 << i) &&
			(allocator->phdmProps.memoryTypes[i].propertyFlags & PROPERTY_FLAGS) ==
			PROPERTY_FLAGS) {
			memoryTypeIndex = i;
			heapIndex = allocator->phdmProps.memoryTypes[i].heapIndex;
			break;
		}
	}

	if (heapIndex == -1) {
		vkma_loge("[vkma] failed to find an appropriate heap\n");
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// bind buffer
	u32 imageSize = imageInfo->extent.width * imageInfo->extent.height * 4;
	VkmaHeap* heap = allocator->heaps + heapIndex;
	for (u32 i = 0; i < heap->blocks.size; i++) {
		VkmaBlock* block = arr_get(&heap->blocks, i);
		if (imageSize < block->availableSize) {
			for (u32 j = 0; j <= block->freeChunks.size; j++) {
				VkmaSubAllocation* chunk = arr_get(&block->freeChunks, j);
				if (imageSize <= chunk->size) {
					// bind buffer
					if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_BIND)) {
						result = vkBindImageMemory(allocator->device, *image,
													block->memory, chunk->offset);
						assert(result == VK_SUCCESS);
					}

					// create allocation
					allocation->heapIndex = heapIndex;
					allocation->blockIndex = i;
					allocation->locale = (VkmaSubAllocation) {
						imageSize, chunk->offset
					};
					allocation->memoryCopy = block->memory;
					allocation->ptr = NULL;
					memset(allocation->name, 0, 64);
					u64 nameLen = strlen(allocInfo->name);
					if (64 <= nameLen) { nameLen = 63; }
					memcpy(allocation->name, allocInfo->name, nameLen);

					// update a chunk
					block->availableSize -= imageSize;
					chunk->size -= imageSize;
					chunk->offset += imageSize;

					vkma_logi("[vkma] Image allocation '%s' created: heapIndex %hu, "
			   				   "blockIndex %hu, size %lu, offset %lu\n",
							  allocation->name, allocation->heapIndex, allocation->blockIndex,
							  allocation->locale.size, allocation->locale.offset);
					
					return VK_SUCCESS;
				}
			}
		}
	}

	if (allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_ALLOCATE) {
		vkma_loge("[vkma] failed to find big enough existing memory block\n");
		return VK_ERROR_UNKNOWN;
	}

	// we will have to create a new memory block
	u64 blockSize = VKMA_DEFAULT_MEMORY_BLOCK_SIZE;
	if (blockSize < imageSize) {
		// make sure blockSize is going to be a multiple of 256 MB
		blockSize = imageSize;
		blockSize += (blockSize % VKMA_DEFAULT_MEMORY_BLOCK_SIZE);
	}
	VkMemoryAllocateInfo memAllocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, blockSize, memoryTypeIndex
	};

	arr_push(&heap->blocks);
	VkmaBlock* tmpBlock = (VkmaBlock*) arr_get(&heap->blocks, heap->blocks.size - 1);
	tmpBlock->size = blockSize;
	tmpBlock->availableSize = blockSize;
	tmpBlock->ptr = NULL;

	result = vkAllocateMemory(allocator->device, &memAllocInfo, NULL, &tmpBlock->memory);
	assert(result);

	arr_init(&tmpBlock->freeChunks, sizeof(VkmaSubAllocation));
	VkmaSubAllocation subAlloc = { tmpBlock->availableSize, 0 };
		
	// bind buffer
	if (!(allocInfo->create & VKMA_ALLOCATION_CREATE_DONT_BIND)) {
		result = vkBindImageMemory(allocator->device, *image, tmpBlock->memory, 0);
		assert(result == VK_SUCCESS);
	}

	// update allocation and freeChunks
   	tmpBlock->availableSize -= imageSize;
	subAlloc.size -= imageSize;
	subAlloc.offset += imageSize;

	arr_add(&tmpBlock->freeChunks, &subAlloc);

	// create allocation
	allocation->heapIndex = heapIndex;
	allocation->blockIndex = heap->blocks.size - 1;
	allocation->locale = (VkmaSubAllocation) { imageSize, 0 };
	allocation->memoryCopy = tmpBlock->memory;
	allocation->ptr = NULL;
	memset(allocation->name, 0, 64);
	u64 nameLen = strlen(allocInfo->name);
	if (64 <= nameLen) { nameLen = 63; }
	memcpy(allocation->name, allocInfo->name, nameLen);

	vkma_logi("[vkma] Image allocation '%s' created: heapIndex %hu, "
			  "blockIndex %hu, size %lu, offset %lu\n",
			  allocation->name, allocation->heapIndex, allocation->blockIndex,
			  allocation->locale.size, allocation->locale.offset);
	
	
	return VK_SUCCESS;
}

VkResult vkmaMapMemory(VkmaAllocator* allocator, VkmaAllocation* allocation, void** ptr)
{
	VkmaHeap* heap = allocator->heaps + allocation->heapIndex;
	VkmaBlock* block = (VkmaBlock*) arr_get(&heap->blocks, allocation->blockIndex);
	VkResult result = VK_SUCCESS;
	if (block->ptr == NULL) {
		result = vkMapMemory(allocator->device, block->memory, 0,
							 VK_WHOLE_SIZE, 0, &block->ptr);
		// assert(result == VK_SUCCESS);
		allocation->ptr = block->ptr + allocation->locale.offset;
		*ptr = allocation->ptr;
	} else {
		allocation->ptr = block->ptr + allocation->locale.offset;
		*ptr = allocation->ptr;
	}
	
	if (result == VK_SUCCESS) {
		vkma_logi("[vkma] Buffer '%s' mapped succesfully to memory at address %x\n",
				  allocation->name, ptr);
	} else {
		vkma_logi("[vkma] Failed to map buffer '%s' to memory\n", allocation->name);
	}
	
	return result;
}

void vkmaUnmapMemory(VkmaAllocator* allocator, VkmaAllocation* allocation)
{
	// TODO: plan this out
	// VkmaHeap* heap = allocator->heaps + allocation->heapIndex;
	// VkmaBlock* block = (VkmaBlock*) arr_get(&heap->blocks, allocation->blockIndex);
	if (allocation->ptr != NULL) { allocation->ptr = NULL; }
	vkma_logi("[vkma] Buffer '%s' unmapped succesfully from memory\n", allocation->name);
}

void vkmaDestroyAllocator(VkmaAllocator* allocator)
{
	for (u32 i = 0; i < allocator->phdmProps.memoryHeapCount; i++) {
		VkmaHeap* heap = allocator->heaps + i;
		for (u32 j = 0; j < heap->blocks.size; j++) {
			VkmaBlock* block = (VkmaBlock*) arr_get(&heap->blocks, j);
			arr_free(&block->freeChunks);
			if (block->ptr != NULL) { vkUnmapMemory(allocator->device, block->memory); }
			vkFreeMemory(allocator->device, block->memory, NULL);
		}
		arr_free(&heap->blocks);
	}
	free(allocator->heaps);
}

void vkmaDestroyBuffer(VkmaAllocator* allocator, VkBuffer* buffer,
					   VkmaAllocation* allocation)
{
	vkDestroyBuffer(allocator->device, *buffer, NULL);
	VkmaSubAllocation* locale = &allocation->locale;
	VkmaHeap* heap = allocator->heaps + allocation->heapIndex;
	VkmaBlock* block = (VkmaBlock*) arr_get(&heap->blocks, allocation->blockIndex);
	for (u32 i = 0; i < block->freeChunks.size; i++) {
		VkmaSubAllocation* subAlloc = (VkmaSubAllocation*) arr_get(&block->freeChunks, i);
		if(locale->offset + locale->size == subAlloc->offset) {
			block->availableSize += locale->size;
			subAlloc->size += locale->size;
			subAlloc->offset = locale->offset;

			vkma_logi("[vkma] Buffer '%s' destroyed\n", allocation->name);

			allocation->heapIndex = 0;
			allocation->blockIndex = 0;
			allocation->locale = (VkmaSubAllocation) { 0, 0 };
			allocation->memoryCopy = VK_NULL_HANDLE;
			allocation->ptr = NULL;
			memset(allocation->name, 0, 64);
			
			return;
		}
	}

	block->availableSize += locale->size;
	VkmaSubAllocation tmp = allocation->locale;
	arr_add(&block->freeChunks, &tmp);

	vkma_logi("[vkma] Buffer '%s' destroyed\n", allocation->name);

	allocation->heapIndex = 0;
	allocation->blockIndex = 0;
	allocation->locale = (VkmaSubAllocation) { 0, 0 };
	allocation->memoryCopy = VK_NULL_HANDLE;
	allocation->ptr = NULL;
	memset(allocation->name, 0, 64);
}

void vkmaDestroyImage(VkmaAllocator* allocator, VkImage* image, VkmaAllocation* allocation)
{
	vkDestroyImage(allocator->device, *image, NULL);
	VkmaSubAllocation* locale = &allocation->locale;
	VkmaHeap* heap = allocator->heaps + allocation->heapIndex;
	VkmaBlock* block = (VkmaBlock*) arr_get(&heap->blocks, allocation->blockIndex);
	for (u32 i = 0; i < block->freeChunks.size; i++) {
		VkmaSubAllocation* subAlloc = (VkmaSubAllocation*) arr_get(&block->freeChunks, i);
		if(locale->offset + locale->size == subAlloc->offset) {
			block->availableSize += locale->size;
			subAlloc->size += locale->size;
			subAlloc->offset = locale->offset;

			vkma_logi("[vkma] Image '%s' destroyed\n", allocation->name);

			allocation->heapIndex = 0;
			allocation->blockIndex = 0;
			allocation->locale = (VkmaSubAllocation) { 0, 0 };
			allocation->memoryCopy = VK_NULL_HANDLE;
			allocation->ptr = NULL;
			memset(allocation->name, 0, 64);
			
			return;
		}
	}

	block->availableSize += locale->size;
	VkmaSubAllocation tmp = allocation->locale;
	arr_add(&block->freeChunks, &tmp);

	vkma_logi("[vkma] Image '%s' destroyed\n", allocation->name);

	allocation->heapIndex = 0;
	allocation->blockIndex = 0;
	allocation->locale = (VkmaSubAllocation) { 0, 0 };
	allocation->memoryCopy = VK_NULL_HANDLE;
	allocation->ptr = NULL;
	memset(allocation->name, 0, 64);
}
