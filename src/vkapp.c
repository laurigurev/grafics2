#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkappc(VkApp* app, Window* window)
{
	vkboilerplatec(&app->boilerplate, window);
	vkcorec(&app->core, &app->boilerplate, window);

	VkPoolInfo pool_infos[2] = {
		(VkPoolInfo) {
			.property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			.size = GIGABYTE
		},
		(VkPoolInfo) {
			.property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.size = GIGABYTE
		}
	};
	
	VkMemoryAllocatorInfo memalloc_info = (VkMemoryAllocatorInfo) {
		.pool_info_count = 2,
		.pool_infos = pool_infos
	};
	vkmemallocc(&app->memalloc, &memalloc_info, &app->boilerplate);
	vkbufferallocc(&app->buffalloc, &app->memalloc, &app->boilerplate);

	VkDescriptorPoolSize dpool_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 10
	};

	VkDescriptorPoolCreateInfo dpool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 10,
		.poolSizeCount = 1,
		.pPoolSizes = &dpool_size
	};

	VkBoilerplate* bp = &app->boilerplate;
	UPDATE_DEBUG_FILE();
	UPDATE_DEBUG_LINE();
	VkResult res = vkCreateDescriptorPool(bp->dev, &dpool_info, NULL, &app->dpool);
	assert(res == VK_SUCCESS);
	logt("VkDescriptorPool created\n");

	vkdoodadc(&app->doodad, &app->buffalloc, &app->memalloc,
			  &app->core, &app->boilerplate, &app->dpool);
	app->current_frame = 0;
}

void vkappd(VkApp* app)
{
	vkDeviceWaitIdle(app->boilerplate.dev);
	vkDestroyDescriptorPool(app->boilerplate.dev, app->dpool, NULL);
	vkdoodadd(&app->doodad, &app->buffalloc, &app->boilerplate);
	vkbufferallocd(&app->buffalloc, &app->boilerplate);
	vkmemallocd(&app->memalloc, &app->boilerplate);
	vkcored(&app->core, &app->boilerplate);
	vkboilerplated(&app->boilerplate);
	flushl();
}

void vkrender(VkApp* app)
{
	VkBoilerplate* bp = &app->boilerplate;
	VkCore* core = &app->core;
	VkDoodad* doodad = &app->doodad;

	UPDATE_DEBUG_FILE();

	UPDATE_DEBUG_LINE();
	vkWaitForFences(bp->dev, 1, core->in_flight + app->current_frame, VK_TRUE, UINT64_MAX);
	uint32_t image_index;
	UPDATE_DEBUG_LINE();
	vkAcquireNextImageKHR(
		bp->dev, core->swapchain, UINT64_MAX,
		core->img_avb[app->current_frame], VK_NULL_HANDLE, &image_index);
	UPDATE_DEBUG_LINE();
	vkResetFences(bp->dev, 1, core->in_flight + app->current_frame);
	
	VkCommandBuffer cmdbuf = core->cmdbuffers[app->current_frame];
	vkResetCommandBuffer(cmdbuf, 0);
	// RECORD COMMAND BUFFERS
	VkCommandBufferBeginInfo cmdbuf_begin_info = (VkCommandBufferBeginInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL
	};
	
	UPDATE_DEBUG_LINE();
	VkResult res = vkBeginCommandBuffer(cmdbuf, &cmdbuf_begin_info);
	assert(res == VK_SUCCESS);
	
	VkClearValue clear_value = (VkClearValue) {
		.color = { { 0.2f, 0.2f, 0.2f, 1.0f } }
	};
	
	VkRenderPassBeginInfo renderpass_begin_info = (VkRenderPassBeginInfo) {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = core->renderpass,
		.framebuffer = core->framebuffers[image_index],
		.renderArea.offset = { 0, 0 },
		.renderArea.extent = core->swpcext,
		.clearValueCount = 1,
		.pClearValues = &clear_value
	};
	
	vkCmdBeginRenderPass(cmdbuf, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	// vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, doodad->pipeline);
	// vkCmdDraw(cmdbuf, 3, 1, 0, 0);
	vkdoodadb(doodad, cmdbuf, app->current_frame);
	vkCmdEndRenderPass(cmdbuf);
	UPDATE_DEBUG_LINE();
	assert(vkEndCommandBuffer(cmdbuf) == VK_SUCCESS);
	
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = (VkSubmitInfo) {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = core->img_avb + app->current_frame,
		.pWaitDstStageMask = &wait_stage,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdbuf,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = core->render_fin + app->current_frame
	};

	UPDATE_DEBUG_LINE();
	res = vkQueueSubmit(bp->queue, 1, &submit_info, core->in_flight[app->current_frame]);
	assert(res == VK_SUCCESS);
	
	VkPresentInfoKHR present_info = (VkPresentInfoKHR) {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = core->render_fin + app->current_frame,
		.swapchainCount = 1,
		.pSwapchains = &core->swapchain,
		.pImageIndices = &image_index,
		.pResults = NULL
	};

	UPDATE_DEBUG_LINE();
    res = vkQueuePresentKHR(bp->queue, &present_info);
	// assert(res == VK_SUCCESS);
	// vkQueuePresentKHR(renderer->queue, &present_info);
	
	app->current_frame++;
	app->current_frame = app->current_frame % MAX_FRAMES_IN_FLIGHT;
}

