#include "grafics2.h"

#define vkbp_logi(...) logi(__VA_ARGS__)
#define vkbp_logd(...) logd(__VA_ARGS__)
#define vkbp_logw(...) logw(__VA_ARGS__)
#define vkbp_loge(...) loge(__VA_ARGS__)

VkResult vkbpCreateMachine(VkbpMachine* machine, u64 size)
{
	machine->pool = malloc(size);
	machine->totalSize = size;
	machine->availableSize = size;
	return VK_SUCCESS;
}

void vkbpDestroyMachine(VkbpMachine* machine)
{
	free(machine->pool);
	machine->totalSize = 0;
	machine->availableSize = 0;
}

u64 vkbpAddBindingPipeline(VkbpMachine* machine, VkbpBindingPipelineInfo* info)
{
	// TODO: realloc

	if (machine->availableSize < 128) {
		machine->pool = realloc(machine->pool, machine->totalSize * 2);
		machine->availableSize += machine->totalSize;
		machine->totalSize *= 2;
	}
	
	u64 offset = machine->totalSize - machine->availableSize;
	assert(0 <= offset);
	void* ptr = machine->pool + offset;
	u64 size = 0;
	u32* cmdCount;

	{
		VkbpInstructionFlag* tmp = ptr + size;
		*tmp = VKBP_INSTRUCTION_START_PIPELINE;
		size += 4;

		cmdCount = ptr + size;
		*cmdCount = 0;
		size += sizeof(u32);
	}
	
	if (info->pipeline == VK_NULL_HANDLE) {
		VkbpInstructionFlag* instr = ptr + size;
		*instr = VKBP_INSTRUCTION_END_PIPELINE;
		size += 4;
		*cmdCount += 1;
		machine->availableSize -= size;
		return offset;
	} else {
		VkbpInstructionFlag* instr = ptr + size;
		*instr = VKBP_INSTRUCTION_BIND_PIPELINE;
		size += 4;
		
		VkPipeline* tmp = ptr + size;
		*tmp = info->pipeline;
		size += sizeof(VkPipeline);

		*cmdCount += 1;
	}

	if (info->vertexBuffer) {
		VkbpInstructionFlag* instr = ptr + size;
		*instr = VKBP_INSTRUCTION_BIND_VERTEX_BUFFER;
		size += 4;
		
		VkBuffer* tmp0 = ptr + size;
		*tmp0 = info->vertexBuffer->buffer;
		size += sizeof(VkBuffer);
		u64* tmp1 = ptr + size;
		*tmp1 = info->vertexBuffer->locale.offset;
		size += sizeof(u64);

		*cmdCount += 1;
	}

	if (info->indexBuffer) {
		VkbpInstructionFlag* instr = ptr + size;
		*instr = VKBP_INSTRUCTION_BIND_INDEX_BUFFER;
		size += 4;
		
		VkBuffer* tmp0 = ptr + size;
		*tmp0 = info->indexBuffer->buffer;
		size += sizeof(VkBuffer);
		u64* tmp1 = ptr + size;
		*tmp1 = info->indexBuffer->locale.offset;
		size += sizeof(u64);

		*cmdCount += 1;
	}

	if (info->pipelineLayout != VK_NULL_HANDLE) {
		VkbpInstructionFlag* instr = ptr + size;
		*instr = VKBP_INSTRUCTION_BIND_DESCRIPTOR_SETS;
		size += 4;

		VkPipelineLayout* tmp0 = ptr + size;
		*tmp0 = info->pipelineLayout;
		size += sizeof(VkPipelineLayout);

		u32* tmp1 = ptr + size;
		*tmp1 = info->maxFramesInFlight;
		size += sizeof(u32);
		
		u32* tmp2 = ptr + size;
		*tmp2 = info->descriptorSetCount;
		size += sizeof(u32);

		VkDescriptorSet* tmp3 = ptr + size;
		memcpy(tmp3, info->descriptorSets, sizeof(VkDescriptorSet) * (*tmp2) * (*tmp1));
		size += sizeof(VkDescriptorSet) * (*tmp2) * (*tmp1);

		/*
		  	NOTE:
			 - descriptor sets are distributed by sequentially for all in the
			   same frame

			      frame 1       frame 2
			   // d0 | d1 | d2 | d3 | d4 | d5 //

		 */

		*cmdCount += 1;
	}

	{
		assert(0 < info->indexCount);
		VkbpInstructionFlag* instr0 = ptr + size;
		*instr0 = VKBP_INSTRUCTION_DRAW_INDEXED;
		size += 4;

		u32* tmp0 = ptr + size;
		*tmp0 = info->indexCount;
		size += sizeof(u32);

		*cmdCount += 1;

		VkbpInstructionFlag* instr1 = ptr + size;
		*instr1 = VKBP_INSTRUCTION_END_PIPELINE;
		size += 4;

		*cmdCount += 1;
	}
	
	machine->availableSize -= size;
	return offset;
}

VkResult vkbpBindBindingPipeline(VkbpMachine* machine, VkCommandBuffer cmd, u32 frame,
								 u64 offset)
{
	void* ptr = machine->pool + offset;
	u64 localOffset = 0;
	VkbpInstructionFlag* start = ptr + localOffset;
	localOffset += sizeof(VkbpInstructionFlag);
	if (!(*start & VKBP_INSTRUCTION_START_PIPELINE)) {
		// TODO: log an error
		return VK_ERROR_UNKNOWN;
	}
	
	u32* cmdCount = ptr + localOffset;
	localOffset += sizeof(u32);
	for (u32 i = 0; i < *cmdCount; i++) {
		VkbpInstructionFlag* instruction = ptr + localOffset;
		localOffset += sizeof(VkbpInstructionFlag);

		/*
		  	TODO:
			 - profile, time or look and assembly translation of different types
			   switch case statements. Try out where you predefine three pointers
			   and cast them when feeding parameters to vulkan binding function
		 */

		switch (*instruction) {
			case VKBP_INSTRUCTION_BIND_PIPELINE:
			{
				VkPipeline* tmp0 = ptr + localOffset;
				localOffset += sizeof(VkPipeline);
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *tmp0);
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_BIND_PIPELINE\n");
				break;
			}
			case VKBP_INSTRUCTION_BIND_VERTEX_BUFFER:
			{
				VkBuffer* tmp0 = ptr + localOffset;
				localOffset += sizeof(VkBuffer);
				u64* tmp1 = ptr + localOffset;
				localOffset += sizeof(u64);
				vkCmdBindVertexBuffers(cmd, 0, 1, tmp0, tmp1);
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_BIND_VERTEX_BUFFER\n");
				break;
			}
			case VKBP_INSTRUCTION_BIND_INDEX_BUFFER:
			{
				VkBuffer* tmp0 = ptr + localOffset;
				localOffset += sizeof(VkBuffer);
				u64* tmp1 = ptr + localOffset;
				localOffset += sizeof(u64);
				vkCmdBindIndexBuffer(cmd, *tmp0, *tmp1, VK_INDEX_TYPE_UINT32);
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_BIND_INDEX_BUFFER\n");
				break;
			}
			case VKBP_INSTRUCTION_BIND_DESCRIPTOR_SETS:
			{
				// REMEMBER: descriptor sets have copies for frames in flight
				
				VkPipelineLayout* tmp0 = (VkPipelineLayout*) (ptr + localOffset);
				localOffset += sizeof(VkPipelineLayout);
				u32* tmp1 = (u32*) (ptr + localOffset);
				localOffset += sizeof(u32);
				u32* tmp2 = (u32*) (ptr + localOffset);
				localOffset += sizeof(u32);
				VkDescriptorSet* tmp3 = ptr + localOffset;
				localOffset += sizeof(VkDescriptorSet) * (*tmp1) * (*tmp2);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *tmp0, 0,
										*tmp2, tmp3 + frame * (*tmp2), 0, NULL);
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_BIND_DESCRIPTOR_SETS\n");
				break;
			}
			case VKBP_INSTRUCTION_DRAW_INDEXED:
			{
				u32* tmp0 = ptr + localOffset;
				localOffset += sizeof(u32);
				vkCmdDrawIndexed(cmd, *tmp0, 1, 0, 0, 0);
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_DRAW_INDEXED\n");
				break;
			}
			case VKBP_INSTRUCTION_END_PIPELINE:
			{
				// vkbp_logi("[vkbp] VKBP_INSTRUCTION_END_PIPELINE\n");
				return VK_SUCCESS;
			}
			default:
			{
				vkbp_loge("[vkbp] unsupported instruction - 0x%x type, number %i, "
						  "Binding Pipeline %lu\n", *instruction, i, offset);
				return VK_ERROR_UNKNOWN;
			}
		}
	}

	vkbp_loge("[vkbp] did not find end instruction for Binding Pipeline %lu\n",
			  (u64) offset);
	return VK_ERROR_UNKNOWN;
}
