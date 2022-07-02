#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkdoodadc(VkDoodad* doodad, VkbaAllocator* bAllocator, VkmaAllocator* mAllocator, 
			   VkCore* core, VkBoilerplate* bp, VkdsManager* dsManager)
{
	UPDATE_DEBUG_FILE();

	vktexturec(&doodad->texture, bp, core, mAllocator, bAllocator);

	doodad->uboData[0] = -0.5f;
	doodad->uboData[1] = -0.5f;
	doodad->uboData[2] = 0.0f;
	VkbaVirtualBufferInfo vBufferInfo = { HOST_INDEX, 3 * 4, doodad->uboData, 1 };
	VkResult res = vkbaCreateVirtualBuffer(bAllocator, doodad->ubos + 0, &vBufferInfo);
	assert(res == VK_SUCCESS);
	res = vkbaCreateVirtualBuffer(bAllocator, doodad->ubos + 1, &vBufferInfo);
	assert(res == VK_SUCCESS);
	memcpy(doodad->ubos[0].dst, doodad->ubos[0].src, doodad->ubos[0].locale.size);
	memcpy(doodad->ubos[1].dst, doodad->ubos[1].src, doodad->ubos[1].locale.size);

	VkdsBindingData bindingData0;
	bindingData0.imageSampler = (VkdsBindingImageSamplerData) {
		doodad->texture.sampler, doodad->texture.view
	};
	VkdsBindingData bindingData1;
	bindingData1.uniformBuffer = (VkdsBindingUniformData) { doodad->ubos };
	VkdsBinding bindings[] = {
		(VkdsBinding) {
			0, VKDS_BINDING_TYPE_IMAGE_SAMPLER, VKDS_BINDING_STAGE_FRAGMENT, bindingData0
		},
		(VkdsBinding) {
		   1, VKDS_BINDING_TYPE_UNIFORM_BUFFER, VKDS_BINDING_STAGE_VERTEX, bindingData1
		}
	};
	VkdsDescriptorSetCreateInfo dsInfo = {
		2, bindings, MAX_FRAMES_IN_FLIGHT
	};
	res = vkdsCreateDescriptorSets(dsManager, &dsInfo, doodad->dsets,
								   &doodad->dlayout);
	assert(res == VK_SUCCESS);

	// -------------------------------------------------------------------
	
	Vertex vertices[4] = {
		{ { -0.5f, -0.5f }, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f }, { 0.0f, 1.0f } },
		{ { -0.5f,  0.5f }, { 1.0f, 1.0f } }
	};
	VkbaVirtualBufferInfo tmpBufferInfo = { DEVICE_INDEX, sizeof(Vertex) * 4, vertices };
	vkbaStageVirtualBuffer(bAllocator, &doodad->vertexbuff, &tmpBufferInfo);

	uint32_t indices[6] = { 0, 1, 3, 1, 2, 3 };
	tmpBufferInfo = (VkbaVirtualBufferInfo) { DEVICE_INDEX, sizeof(u32) * 6, indices };
	vkbaStageVirtualBuffer(bAllocator, &doodad->indexbuff, &tmpBufferInfo);

	vec2f instance_data[] = {
		(vec2f) { 0.0f, 0.0f },
		(vec2f) { 1.0f, 0.0f },
		(vec2f) { 0.0f, 1.0f }
	};
	tmpBufferInfo = (VkbaVirtualBufferInfo) {
		DEVICE_INDEX, sizeof(vec2f) * 3, instance_data
	};
	vkbaStageVirtualBuffer(bAllocator, &doodad->instbuff, &tmpBufferInfo);

	VkVertexInputBindingDescription vibd[] = {
		(VkVertexInputBindingDescription) {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},
		(VkVertexInputBindingDescription) {
			.binding = 1,
			.stride = sizeof(vec2f),
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
		}
	};

	VkVertexInputAttributeDescription viad[] = {
		(VkVertexInputAttributeDescription) {
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0
		},
		(VkVertexInputAttributeDescription) {
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 2*4
		},
		(VkVertexInputAttributeDescription) {
			.binding = 1,
			.location = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0
		}
	};

	VkenPipelineCreateInfo pInfo = {
		"spv/default.vert.spv", "spv/default.frag.spv", 2, vibd, 3, viad,
		1, &doodad->dlayout, core->swpcext, bp->dev, core->renderpass, 0
	};
	vkenCreatePipelines(1, &doodad->pipeline, &pInfo);
}

void vkdoodadd(VkDoodad* doodad, VkbaAllocator* bAllocator,
			   VkBoilerplate* bp, VkmaAllocator* mAllocator)
{
	UPDATE_DEBUG_FILE();

	vkenDestroyPipeline(&doodad->pipeline);

	vkbaDestroyVirtualBuffer(bAllocator, &doodad->vertexbuff);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->indexbuff);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->instbuff);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->ubos[0]);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->ubos[1]);

	vktextured(&doodad->texture, bp, mAllocator);

	logt("VkDoodad destroyed\n");
}

