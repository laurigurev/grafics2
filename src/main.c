#include "grafics2.h"

int main()
{
	log_init("grafics2.log");
	logi("hello, vulkan!\n");

	/*
	TrueTypeFont* ttf = NULL;
	i32 result = ttf_load2(&ttf, "resources/calibri.ttf");
	assert(result == 1);
	ttf_free2(&ttf);
	logi("[main] ttf loaded and freed\n");
	*/

	Window win;
	windowc(&win, "grafics2.exe", 750, 750);

	// VkApp* app = (VkApp*) malloc(sizeof(VkApp));
	VkApp app;
	vkappc(&app, &win);
	
	while (!win.should_close)
	{
		wpoll_events(&win);
		vkrender(&app);
	}

	vkappd(&app);
	windowd(&win);
	
	log_close();
	
	return 0;
}
