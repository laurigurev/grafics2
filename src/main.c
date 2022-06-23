#include "grafics2.h"

int main()
{
	log_init("grafics2.log");
	logi("hello, vulkan!\n");

	// ttf_core ttf;
	// ttf_load(&ttf);
	// char* bmp = (char*) ttf_to_bmp(64, 64, &ttf);
	// ttf_free(&ttf);
	// free(bmp);
	
	Window win;
	windowc(&win, "grafics2.exe", 750, 750);

	VkApp* app = (VkApp*) malloc(sizeof(VkApp));
	vkappc(app, &win);
	
	while (!win.should_close)
	{
		wpoll_events(&win);
		vkrender(app);
	}

	vkappd(app);
	windowd(&win);
	
	log_close();
	
	return 0;
}
