#include "grafics2.h"

#define vkds_logi(...) logi(__VA_ARGS__)
#define vkds_logd(...) logd(__VA_ARGS__)
#define vkds_logw(...) logw(__VA_ARGS__)
#define vkds_loge(...) loge(__VA_ARGS__)

VkResult vkdsCreateManager(VkdsManager* manager, VkdsManagerCreateInfo* info)
{
	assert(manager != NULL);
	assert(info != NULL);
	
	manager->deviceCopy = info->device;
	
	VkDescriptorPoolSize descPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info->poolSize },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info->poolSize },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, info->poolSize },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, info->poolSize }
	};

	// TODO: translate flags into correct form
	VkDescriptorPoolCreateInfo descPoolInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, NULL,
		info->flags, 4 * info->poolSize, 4, descPoolSizes
	};

	VkResult result = vkCreateDescriptorPool(manager->deviceCopy, &descPoolInfo,
											 NULL, &manager->descPool);

	arr_init(&manager->descSetLayouts, sizeof(VkDescriptorSetLayout));
	vkds_logi("[vkds] Vulkan descriptor set manager created\n");
	return result;
}

VkResult vkdsCreateDescriptorSets(VkdsManager* manager, VkdsDescriptorSetCreateInfo* info,
								 VkDescriptorSet* descriptorSets)
{
	assert(manager != NULL);
	assert(info != NULL);
	assert(descriptorSets != NULL);
	VkResult result;
	
	VkDescriptorType descTypesLUTs[] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
	VkShaderStageFlags shaderStagesLUTs[] = {
		VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorType descType[info->bindingCount];
	VkShaderStageFlags shaderStage[info->bindingCount];
	
	VkDescriptorSetLayoutBinding descSetLayoutBindings[info->bindingCount];
	VkdsBinding* binding = info->bindings;
	for (u32 i = 0; i < info->bindingCount; i++) {
		switch (binding->type) {
			case VKDS_BINDING_TYPE_IMAGE_SAMPLER:
				descType[i] = descTypesLUTs[0];
				break;
			default:
				vkds_loge("[vkds] in binding %i, type %i not found\n", i, binding->type);
				// flushl();
				return VK_ERROR_UNKNOWN;
		};
		
		switch (binding->stage) {
			case VKDS_BINDING_STAGE_VERTEX:
				shaderStage[i] = shaderStagesLUTs[0];
				break;
			case VKDS_BINDING_STAGE_FRAGMENT:
				shaderStage[i] = shaderStagesLUTs[1];
				break;
			default:
				vkds_loge("[vkds] in binding %i, stage %i not found\n", i, binding->stage);
				// flushl();
				return VK_ERROR_UNKNOWN;
		};
		
		descSetLayoutBindings[i] = (VkDescriptorSetLayoutBinding) {
			binding->location, descType[i], 1, shaderStage[i], NULL
		};
		binding++;
	}

	VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0,
		info->bindingCount, descSetLayoutBindings
	};

	VkDescriptorSetLayout tmpDescSetLayout;
	result = vkCreateDescriptorSetLayout(manager->deviceCopy, &descSetLayoutInfo,
										 NULL, &tmpDescSetLayout);
	if (result != VK_SUCCESS) { return result; }
	// arr_add(&manager->descSetLayouts, tmpDescSetLayout);

	VkDescriptorSetAllocateInfo descSetAllocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, manager->descPool,
		1, &tmpDescSetLayout
	};
	
	// --------------------------------------------------------------------------------

	for (u32 i = 0; i < info->framesInFlight; i++) {
		result = vkAllocateDescriptorSets(manager->deviceCopy, &descSetAllocInfo,
										  descriptorSets + i);
		if (result != VK_SUCCESS) { return result; }
	
		binding = info->bindings;
		VkWriteDescriptorSet descSetWrites[info->bindingCount];
		Array descriptorImageInfos;
		arr_init(&descriptorImageInfos, sizeof(VkDescriptorImageInfo));
		/*
		  Array descriptorBufferInfos;
		  arr_init(&descriptorBufferInfos, sizeof(VkDescriptorBufferInfo));
		*/
		for (u32 j = 0; j < info->bindingCount; j++) {
			descSetWrites[j] = (VkWriteDescriptorSet) {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, descriptorSets[i],
				binding[j].location, 0, 1, descType[j], NULL, NULL, NULL
			};

			if (descType[j] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				VkdsBindingImageSamplerData tmpImageSampler = binding->data.imageSampler;
				VkDescriptorImageInfo tmpDescriptorImageInfo = {
					tmpImageSampler.sampler, tmpImageSampler.view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};
				arr_add(&descriptorImageInfos, &tmpDescriptorImageInfo);
				descSetWrites[j].pImageInfo = arr_get(&descriptorImageInfos,
													  descriptorImageInfos.size - 1);
			} else {
				assert(false);
			} /* else if (descType[i] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) { */
			binding++;
		}
		vkUpdateDescriptorSets(manager->deviceCopy, info->bindingCount, descSetWrites,
							   0, VK_NULL_HANDLE);
		arr_free(&descriptorImageInfos);
	}
	
	arr_add(&manager->descSetLayouts, &tmpDescSetLayout);
	vkds_logi("[vkds] %hu Descriptor sets created\n", info->framesInFlight);
	return result;
}

void vkdsDestroyManager(VkdsManager* manager)
{
	assert(manager != NULL);
	
	for (u32 i = 0; i < manager->descSetLayouts.size; i++) {
		VkDescriptorSetLayout* descriptorSetLayout = arr_get(&manager->descSetLayouts, i);
		vkDestroyDescriptorSetLayout(manager->deviceCopy, *descriptorSetLayout, NULL);
	}
	arr_free(&manager->descSetLayouts);
	vkDestroyDescriptorPool(manager->deviceCopy, manager->descPool, NULL);
	manager->deviceCopy = VK_NULL_HANDLE;
	vkds_logi("[vkds] Vulkan descriptor set manager destroyed\n");
}
