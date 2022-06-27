#include "grafics2.h"

#define vken_logi(...) logi(__VA_ARGS__)
#define vken_logd(...) logd(__VA_ARGS__)
#define vken_logw(...) logw(__VA_ARGS__)
#define vken_loge(...) loge(__VA_ARGS__)

VkResult vkenCreatePipelines(u32 pipeline_count, VkenPipeline* pipelines,
							 VkenPipelineCreateInfo* infos)
{
	/*
	  NOTE:
	   - pipelines have to be already allocated, this creation function does not
	     allocate pipelines
     */
	
	assert(pipelines != NULL);
	assert(infos != NULL);
	assert(pipeline_count != 0);

	VkResult result;

	VkGraphicsPipelineCreateInfo* pipeline_infos = malloc(sizeof(
										   VkGraphicsPipelineCreateInfo) * pipeline_count);
	VkPipelineShaderStageCreateInfo* shader_stages = malloc(
						 sizeof(VkPipelineShaderStageCreateInfo) * pipeline_count * 2);
	VkShaderModule* shader_modules = malloc(sizeof(VkShaderModule) * pipeline_count * 2);
	VkPipelineVertexInputStateCreateInfo* vertex_input_states = malloc(
						  sizeof(VkPipelineVertexInputStateCreateInfo) * pipeline_count);
	VkPipelineLayout* layouts = malloc(sizeof(VkPipelineLayout) * pipeline_count);
	VkPipeline* tmp_pipelines = malloc(sizeof(VkPipeline) * pipeline_count);

	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info =
		(VkPipelineInputAssemblyStateCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	
	// --------------------------------------------------------------------------------

	VkViewport viewport = (VkViewport) {
		.x = 0.0f,
		.y = 0.0f,
		.width = infos[0].extent.width,
		.height = infos[0].extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	
	VkRect2D scissor = (VkRect2D) {
		.offset = (VkOffset2D) { .x = 0, .y = 0 },
		.extent = infos[0].extent
	};
	
	VkPipelineViewportStateCreateInfo viewport_state_info =
		(VkPipelineViewportStateCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};
	
	// --------------------------------------------------------------------------------

	VkPipelineRasterizationStateCreateInfo rasterization_state_info =
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

	// --------------------------------------------------------------------------------

	VkPipelineMultisampleStateCreateInfo multisample_state_info =
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
	
	// --------------------------------------------------------------------------------

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info =
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
	
	// --------------------------------------------------------------------------------

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
	
	VkPipelineColorBlendStateCreateInfo color_blend_state_info =
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

	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	for(u32 i = 0; i < pipeline_count; i++) {

		pipelines[i].device_copy = infos[i].device;

		uint32_t vertsize, fragsize;
		char* vertex_shader = file_read(infos[i].vertex_shader_path, &vertsize);
		char* fragment_shader = file_read(infos[i].fragment_shader_path, &fragsize);

		VkShaderModuleCreateInfo shader_module_info = (VkShaderModuleCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.codeSize = vertsize,
			.pCode = (uint32_t*) vertex_shader
		};

		result = vkCreateShaderModule(pipelines[i].device_copy, &shader_module_info,
									  NULL, shader_modules + i * 2 + 0);
		if (result != VK_SUCCESS) {
			vken_loge("[vken - i %i] failed to create vertex shader module\n", i);
			return result;
		}
		
		shader_stages[i * 2 + 0] = (VkPipelineShaderStageCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shader_modules[i * 2 + 0],
			.pName = "main",
			.pSpecializationInfo = NULL
		};
		
		shader_module_info.codeSize = fragsize;
		shader_module_info.pCode = (uint32_t*) fragment_shader;

		result = vkCreateShaderModule(pipelines[i].device_copy, &shader_module_info,
									  NULL, shader_modules + i * 2 + 1);
		if (result != VK_SUCCESS) {
			vken_loge("[vken - i %] failed to create fragment shader module\n", i);
			return result;
		}

		shader_stages[i * 2 + 1] = (VkPipelineShaderStageCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shader_modules[i * 2 + 1],
			.pName = "main",
			.pSpecializationInfo = NULL
		};

		file_free(vertex_shader);
		file_free(fragment_shader);

		// --------------------------------------------------------------------------------

		// VkPipelineVertexInputStateCreateInfo vertex_input_state_info =
		vertex_input_states[i] = (VkPipelineVertexInputStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.vertexBindingDescriptionCount = infos[i].vertex_input_binding_description_count,
			.pVertexBindingDescriptions = infos[i].vertex_input_binding_descriptions,
			.vertexAttributeDescriptionCount =
					infos[i].vertex_input_attribute_description_count,
			.pVertexAttributeDescriptions = infos[i].vertex_input_attribute_descriptions
		};
	
		// --------------------------------------------------------------------------------

		VkPipelineLayoutCreateInfo pipeline_layout_info = (VkPipelineLayoutCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.setLayoutCount = infos[i].descriptor_set_layout_count,
			.pSetLayouts = infos[i].layouts,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = NULL
		};

		result = vkCreatePipelineLayout(pipelines[i].device_copy, &pipeline_layout_info,
												 NULL, layouts + i);
		if (result != VK_SUCCESS) {
			vken_loge("[vken - i %i] failed to create pipeline layout\n", i);
			return result;
		}
	
		// --------------------------------------------------------------------------------

		pipeline_infos[i] = (VkGraphicsPipelineCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stageCount = 2,
			.pStages = shader_stages + i * 2,
			.pVertexInputState = vertex_input_states + i,
			.pInputAssemblyState = &input_assembly_state_info,
			.pTessellationState = NULL,
			.pViewportState = &viewport_state_info,
			.pRasterizationState = &rasterization_state_info,
			.pMultisampleState = &multisample_state_info,
			.pDepthStencilState = &depth_stencil_state_info,
			.pColorBlendState = &color_blend_state_info,
			.pDynamicState = NULL,
			.layout = layouts[i],
			.renderPass = infos[i].renderpass,
			.subpass = infos[i].subpass_index,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};
	}

	result = vkCreateGraphicsPipelines(pipelines[0].device_copy, VK_NULL_HANDLE,
									   pipeline_count, pipeline_infos, NULL, tmp_pipelines);
	if (result != VK_SUCCESS) {
		vken_loge("[vken] failed to create graphics pipelines\n");
		return result;
	}

	for (u32 i = 0; i < pipeline_count; i++) {
		vkDestroyShaderModule(pipelines[i].device_copy, shader_modules[i * 2 + 0], NULL);
		vkDestroyShaderModule(pipelines[i].device_copy, shader_modules[i * 2 + 1], NULL);
		pipelines[i].pipe = tmp_pipelines[i];
		pipelines[i].layout = layouts[i];
	}

	free(pipeline_infos);
	free(shader_stages);
	free(shader_modules);
	free(vertex_input_states);
	free(layouts);
	free(tmp_pipelines);

	return VK_SUCCESS;
}

void vkenDestroyPipeline(VkenPipeline* pipeline)
{
	assert(pipeline != NULL);
	vkDestroyPipelineLayout(pipeline->device_copy, pipeline->layout, NULL);
	vkDestroyPipeline(pipeline->device_copy, pipeline->pipe, NULL);
}
