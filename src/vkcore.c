#include "grafics2.h"

#define UPDATE_DEBUG_LINE() bp->user_data.line = __LINE__ + 1
#define UPDATE_DEBUG_FILE() bp->user_data.file = __FILE__

void vkcorec(VkCore* core, VkBoilerplate* bp, Window* win)
{
	UPDATE_DEBUG_FILE();
	
	VkSurfaceCapabilitiesKHR surface_capabs;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(bp->phydev, bp->surface, &surface_capabs);
	
	uint32_t img_count = surface_capabs.minImageCount + 1;
	if (img_count > surface_capabs.maxImageCount)
	{
		img_count = surface_capabs.maxImageCount;
	}
	
	VkExtent2D swapchain_extent;
	if (surface_capabs.currentExtent.width == -1 || surface_capabs.currentExtent.height == -1)
	{
		uint32_t width, height;
		wframebuffer_size(win, &width, &height);
		swapchain_extent = (VkExtent2D) { width, height };
	}
	else
	{
		swapchain_extent = surface_capabs.currentExtent;
	}
	core->swpcext = swapchain_extent;
	
	uint32_t queue_fam_index = 0;
	
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		bp->phydev, bp->surface, &present_mode_count, NULL);
	VkPresentModeKHR* present_modes = (VkPresentModeKHR*) malloc(
    	sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		bp->phydev, bp->surface, &present_mode_count, present_modes);
	
	VkPresentModeKHR current_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	VkPresentModeKHR* present_modes_temporary = present_modes;
	present_mode_count--;
	while (present_mode_count--)
	{
		if (*present_modes_temporary == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			current_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		
		present_modes_temporary++;
	}
	free(present_modes);
	
	VkSwapchainCreateInfoKHR swapchain_info = (VkSwapchainCreateInfoKHR) {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = bp->surface,
		.minImageCount = img_count,
		.imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,		// could be something fancy
		.imageExtent = swapchain_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queue_fam_index,
		.preTransform = surface_capabs.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = current_present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	UPDATE_DEBUG_LINE();
	VkResult res = vkCreateSwapchainKHR(bp->dev, &swapchain_info, NULL, &core->swapchain);
	assert(res == VK_SUCCESS);
	logt("VkSwapchainKHR created\n");

	// -----------------------------------------------------------

	UPDATE_DEBUG_LINE();
	vkGetSwapchainImagesKHR(bp->dev, core->swapchain, &core->swpcimg_count, NULL);
	core->swpcimgs = (VkImage*) malloc(sizeof(VkImage) * core->swpcimg_count);
	vkGetSwapchainImagesKHR(bp->dev, core->swapchain, &core->swpcimg_count, core->swpcimgs);
	
	core->swpcviews = (VkImageView*) malloc(sizeof(VkImageView) * core->swpcimg_count);
	
	VkImageViewCreateInfo swapchain_view_info = (VkImageViewCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = NULL,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_SRGB,
		.components = (VkComponentMapping) { 0, 0, 0, 0 },
		.subresourceRange = (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};
	
	for (uint32_t i = 0; i < core->swpcimg_count; i++)
	{
		swapchain_view_info.image = core->swpcimgs[i];
		UPDATE_DEBUG_LINE();
		vkCreateImageView(bp->dev, &swapchain_view_info, NULL, core->swpcviews + i);
	}

	logt("swapchain VkImageViews created\n"); 
	
	// -----------------------------------------------------------

	VkAttachmentDescription attachment = (VkAttachmentDescription) {
		.flags = 0,
	   	.format = VK_FORMAT_B8G8R8A8_SRGB,
   		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	   	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
   		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	   	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference attachment_reference = (VkAttachmentReference) {
	   	.attachment = 0,
   		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = (VkSubpassDescription) {
   		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	   	.inputAttachmentCount = 0,
   		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
	   	.pColorAttachments = &attachment_reference,
   		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
	   	.preserveAttachmentCount = 0,
   		.pPreserveAttachments = NULL
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkRenderPassCreateInfo renderpass_info = (VkRenderPassCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	UPDATE_DEBUG_LINE();
	res = vkCreateRenderPass(bp->dev, &renderpass_info, NULL, &core->renderpass);
	assert(res == VK_SUCCESS);
	logt("VkRenderPass created\n");

	// -----------------------------------------------------------

	core->framebuffers = (VkFramebuffer*) malloc(sizeof(VkFramebuffer) * core->swpcimg_count);
	VkFramebufferCreateInfo framebuffer_info = (VkFramebufferCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.renderPass = core->renderpass,
		.attachmentCount = 1,
		.pAttachments = NULL,
		.width = swapchain_extent.width,
		.height = swapchain_extent.height,
		.layers = 1
	};

	for (uint32_t i = 0; i < core->swpcimg_count; i++)
	{
   		framebuffer_info.pAttachments = core->swpcviews + i;
		UPDATE_DEBUG_LINE();	
		res = vkCreateFramebuffer(bp->dev, &framebuffer_info, NULL, core->framebuffers + i);
		assert(res == VK_SUCCESS);
		logt("VkFramebuffer created\n");
	}
	
	// -----------------------------------------------------------

	VkCommandPoolCreateInfo cmdpool_info = (VkCommandPoolCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
				 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = 0
	};

	UPDATE_DEBUG_LINE();
	res = vkCreateCommandPool(bp->dev, &cmdpool_info, NULL, &core->cmdpool);
	assert(res == VK_SUCCESS);
	logt("VkCommandPool created\n");

	// -----------------------------------------------------------

	core->cmdbuffers = (VkCommandBuffer*) malloc(
		sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo cmdbuffer_alloc_info = (VkCommandBufferAllocateInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = core->cmdpool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT
	};

	UPDATE_DEBUG_LINE();
	res = vkAllocateCommandBuffers(bp->dev, &cmdbuffer_alloc_info, core->cmdbuffers);
	assert(res == VK_SUCCESS);
	logt("VkCommandBuffers allocated\n");
	
	// -----------------------------------------------------------

	core->img_avb = (VkSemaphore*) malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	core->render_fin = (VkSemaphore*) malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	core->in_flight = (VkFence*) malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info= (VkSemaphoreCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};
	
	VkFenceCreateInfo fence_info = (VkFenceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		UPDATE_DEBUG_LINE();
		res = vkCreateSemaphore(bp->dev, &semaphore_info, NULL, core->img_avb + i);
		assert(res == VK_SUCCESS);
		logt("VkSemaphore created\n");

		UPDATE_DEBUG_LINE();
		res = vkCreateSemaphore(bp->dev, &semaphore_info, NULL, core->render_fin + i);
		assert(res == VK_SUCCESS);
		logt("VkSemaphore created\n");

		UPDATE_DEBUG_LINE();
		res = vkCreateFence(bp->dev, &fence_info, NULL, core->in_flight + i);
		assert(res == VK_SUCCESS);
		logt("VkFence created\n");
	}
}

void vkcored(VkCore* core, VkBoilerplate* bp)
{
	UPDATE_DEBUG_FILE();
	
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		UPDATE_DEBUG_LINE();
		vkDestroySemaphore(bp->dev, core->img_avb[i], NULL);
		logt("VkSemaphore destroyed\n");

		UPDATE_DEBUG_LINE();
		vkDestroySemaphore(bp->dev, core->render_fin[i], NULL);
		logt("VkSemaphore destroyed\n");
		
		UPDATE_DEBUG_LINE();
		vkDestroyFence(bp->dev, core->in_flight[i], NULL);
		logt("VkFence destroyed\n");
	}

	free(core->img_avb);
	free(core->render_fin);
	free(core->in_flight);
	logt("semaphores and fence freed\n");
	
	UPDATE_DEBUG_LINE();
	vkDestroyCommandPool(bp->dev, core->cmdpool, NULL);
	logt("VkCommandPool destroyed\n");
	
	for (uint32_t i = 0; i < core->swpcimg_count; i++)
	{
		UPDATE_DEBUG_LINE();
		vkDestroyFramebuffer(bp->dev, core->framebuffers[i], NULL);
		logt("VkFramebuffer destroyed\n");
	}
	
	UPDATE_DEBUG_LINE();
	vkDestroyRenderPass(bp->dev, core->renderpass, NULL);
	logt("VkRenderPass destroyed\n");

	for (uint32_t i = 0; i < core->swpcimg_count; i++)
	{
		UPDATE_DEBUG_LINE();
		vkDestroyImageView(bp->dev, core->swpcviews[i], NULL);
	}
	logt("swapchain VkImageViews destroyed\n");

	free(core->swpcimgs);
	free(core->swpcviews);

	logt("swapchain images and views freed\n");
	
	UPDATE_DEBUG_LINE();
	vkDestroySwapchainKHR(bp->dev, core->swapchain, NULL);
	logt("VkSwapchainKHR destroyed\n");
}
