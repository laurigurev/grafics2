#include "grafics2.h"

int main()
{
	log_init("grafics2.log");
	logi("hello, vulkan!\n");
	
	Window win;
	windowc(&win, "grafics2.exe", 750, 500);

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
