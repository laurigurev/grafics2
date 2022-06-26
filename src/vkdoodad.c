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
	
	uint32_t vertsize, fragsize;
	char* vertex_shader = file_read("spv/default.vert.spv", &vertsize);
	char* fragment_shader = file_read("spv/default.frag.spv", &fragsize);
	
	VkPipelineShaderStageCreateInfo shader_stage_infos[2];
	VkShaderModule shader_modules[2];
		
	VkShaderModuleCreateInfo shader_module_info = (VkShaderModuleCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	   	.pNext = NULL,
   		.flags = 0,
		.codeSize = vertsize,
	   	.pCode = (uint32_t*) vertex_shader
   	};

	UPDATE_DEBUG_LINE();
	res = vkCreateShaderModule(
		bp->dev, &shader_module_info, NULL, shader_modules + 0);
	assert(res == VK_SUCCESS);
		
	shader_stage_infos[0] = (VkPipelineShaderStageCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	   	.pNext = NULL,
   		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
	   	.module = shader_modules[0],
   		.pName = "main",
		.pSpecializationInfo = NULL
	};
		
	shader_module_info.codeSize = fragsize;
	shader_module_info.pCode = (uint32_t*) fragment_shader;

	UPDATE_DEBUG_LINE();
	res = vkCreateShaderModule(bp->dev, &shader_module_info, NULL, shader_modules + 1);
	assert(res == VK_SUCCESS);
		
	shader_stage_infos[1] = (VkPipelineShaderStageCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = shader_modules[1],
		.pName = "main",
		.pSpecializationInfo = NULL
	};

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

	VkVertexInputBindingDescription vibd = (VkVertexInputBindingDescription) {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription viad[2] = {
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
		}
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_info =
		(VkPipelineVertexInputStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vibd,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = viad
	};
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info =
		(VkPipelineInputAssemblyStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
	};
	
	VkViewport viewport = (VkViewport) {
		.x = 0.0f,
		.y = 0.0f,
		.width = core->swpcext.width,
		.height = core->swpcext.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	
	VkRect2D scissor = (VkRect2D) {
		.offset = (VkOffset2D) { .x = 0, .y = 0 },
		.extent = core->swpcext
	};
	
	VkPipelineViewportStateCreateInfo viewport_info = (VkPipelineViewportStateCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};
	
	VkPipelineRasterizationStateCreateInfo rasterization_info =
		(VkPipelineRasterizationStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
	   		.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
	};
	
	VkPipelineMultisampleStateCreateInfo multisample_info =
		(VkPipelineMultisampleStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
	    	.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.0f,
			.pSampleMask = NULL,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
	};
	
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info =
		(VkPipelineDepthStencilStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
	};
	
	VkPipelineColorBlendAttachmentState color_blend_attachment_state =
		(VkPipelineColorBlendAttachmentState) {
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
							  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	
	VkPipelineColorBlendStateCreateInfo color_blend_info =
		(VkPipelineColorBlendStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment_state,
			.blendConstants[0] = 0.0f,
			.blendConstants[1] = 0.0f,
			.blendConstants[2] = 0.0f,
			.blendConstants[3] = 0.0f
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = (VkPipelineLayoutCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &doodad->dlayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

	UPDATE_DEBUG_LINE();
	vkCreatePipelineLayout(bp->dev, &pipeline_layout_info, NULL, &doodad->pipeline_layout);
	logt("VkPipelineLayout created\n");

	VkGraphicsPipelineCreateInfo pipeline_info = (VkGraphicsPipelineCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	   	.pNext = NULL,
   		.flags = 0,
		.stageCount = 2,
	   	.pStages = shader_stage_infos,
   		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
	   	.pTessellationState = NULL,
   		.pViewportState = &viewport_info,
		.pRasterizationState = &rasterization_info,
	   	.pMultisampleState = &multisample_info,
   		.pDepthStencilState = &depth_stencil_info,
		.pColorBlendState = &color_blend_info,
	   	.pDynamicState = NULL,
   		.layout = doodad->pipeline_layout,
		.renderPass = core->renderpass,
	   	.subpass = 0,
   		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	UPDATE_DEBUG_LINE();
	res = vkCreateGraphicsPipelines(
		bp->dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &doodad->pipeline);
	assert(res == VK_SUCCESS);
	logt("VkPipeline created\n");

	vkDestroyShaderModule(bp->dev, shader_modules[0], NULL);
	vkDestroyShaderModule(bp->dev, shader_modules[1], NULL);	

	file_free(vertex_shader);
	file_free(fragment_shader);
}

void vkdoodadd(VkDoodad* doodad, VkbaAllocator* bAllocator,
			   VkBoilerplate* bp, VkmaAllocator* mAllocator)
{
	UPDATE_DEBUG_FILE();

	UPDATE_DEBUG_LINE();
	vkDestroyPipelineLayout(bp->dev, doodad->pipeline_layout, NULL);
	logt("VkPipelineLayout destroyed\n");

	UPDATE_DEBUG_LINE();
	vkDestroyPipeline(bp->dev, doodad->pipeline, NULL);
	logt("VkPipeline destroyed\n");

	vkbaDestroyVirtualBuffer(bAllocator, &doodad->vertexbuff);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->indexbuff);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->ubos[0]);
	vkbaDestroyVirtualBuffer(bAllocator, &doodad->ubos[1]);

	vktextured(&doodad->texture, bp, mAllocator);

	logt("VkDoodad destroyed\n");
}

