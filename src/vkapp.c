#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkappc(VkApp* app, Window* window)
{
	vkboilerplatec(&app->boilerplate, window);
	vkcorec(&app->core, &app->boilerplate, window);
	vkdoodadc(&app->doodad, &app->core, &app->boilerplate);

	app->current_frame = 0;
}

void vkappd(VkApp* app)
{
	vkDeviceWaitIdle(app->boilerplate.dev);
	vkdoodadd(&app->doodad, &app->boilerplate);
	vkcored(&app->core, &app->boilerplate);
	vkboilerplated(&app->boilerplate);
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
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, doodad->pipeline);
	vkCmdDraw(cmdbuf, 3, 1, 0, 0);
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

