#include "grafics2.h"

int main()
{
	log_init("grafics2.log");
	logi("hello, vulkan!\n");

	TrueTypeFont* ttf = NULL;
	i32 result = ttf_load2(&ttf, "resources/calibri.ttf");
	assert(result == 1);

	/*
	int iters = 10000;
	int time;
	time_t before = clock();
	for (u32 i = 0; i < iters; i++) {
		void* bmp = ttf_create_bitmap0(ttf, 'a', 128, 128); free(bmp);
	}
	time_t difference = clock() - before;
	time = difference * 1000 / CLOCKS_PER_SEC;
	logd("[ttf_create_bitmap0] time %i, iterations %i\n", time, iters);

	before = clock();
	for (u32 i = 0; i < iters; i++) {
		void* bmp = ttf_create_bitmap1(ttf, 'a', 128, 128); free(bmp);
	}
	difference = clock() - before;
	time = difference * 1000 / CLOCKS_PER_SEC;
	logd("[ttf_create_bitmap1] time %i, iterations %i\n", time, iters);

	before = clock();
	for (u32 i = 0; i < iters; i++) {
		void* bmp = ttf_create_bitmap2(ttf, 'a', 128, 128); free(bmp);
	}
	difference = clock() - before;
	time = difference * 1000 / CLOCKS_PER_SEC;
	logd("[ttf_create_bitmap2] time %i, iterations %i\n", time, iters);
	*/
	
	ttf_free2(&ttf);
	logi("[main] ttf loaded and freed\n");

	Window win;
	windowc(&win, "grafics2.exe", 750, 750);

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
