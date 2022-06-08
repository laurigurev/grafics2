cc := gcc
glslc := glslc.exe
exe := a.exe
vulkan_inc := -IC:/VulkanSDK/1.3.204.1/Include/vulkan
vulkan_lib := -LC:/VulkanSDK/1.3.204.1/Lib -lvulkan-1
win32_lib := -lgdi32 -luser32 -lkernel32 -lcomctl32 -lm -mwindows
libs := $(vulkan_lib) $(win32_lib)
flags := -g -Wall -O0 -DVK_USE_PLATFORM_WIN32_KHR

all: spv/default.vert.spv spv/default.frag.spv obj/main.o obj/logger.o obj/vkboilerplate.o obj/vkdebug.o obj/win32.o obj/vkcore.o obj/fileio.o obj/vkdoodad.o obj/vkpool.o obj/vkmemory.o obj/vkbufferalloc.o obj/bmploader.o obj/vktexture.o obj/vkapp.o del $(exe)

spv/default.vert.spv: shaders/default.vert
	$(glslc) $? -o $@

spv/default.frag.spv: shaders/default.frag
	$(glslc) $? -o $@

obj/main.o: src/main.c
	$(cc) $(vulkan_inc) $(flags) -c src/main.c -o obj/main.o

obj/logger.o: src/logger.c
	$(cc) $(vulkan_inc) $(flags) -c src/logger.c -o obj/logger.o

obj/vkboilerplate.o: src/vkboilerplate.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkboilerplate.c -o obj/vkboilerplate.o

obj/vkdebug.o: src/vkdebug.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkdebug.c -o obj/vkdebug.o

obj/win32.o: src/win32.c
	$(cc) $(vulkan_inc) $(flags) -c src/win32.c -o obj/win32.o

obj/vkcore.o: src/vkcore.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkcore.c -o obj/vkcore.o

obj/fileio.o: src/fileio.c
	$(cc) $(vulkan_inc) $(flags) -c src/fileio.c -o obj/fileio.o

obj/vkdoodad.o: src/vkdoodad.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkdoodad.c -o obj/vkdoodad.o

obj/vkpool.o: src/vkpool.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkpool.c -o obj/vkpool.o

obj/vkmemory.o: src/vkmemory.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkmemory.c -o obj/vkmemory.o

obj/vkbufferalloc.o: src/vkbufferalloc.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkbufferalloc.c -o obj/vkbufferalloc.o

obj/bmploader.o: src/bmploader.c
	$(cc) $(vulkan_inc) $(flags) -c src/bmploader.c -o obj/bmploader.o

obj/vktexture.o: src/vktexture.c
	$(cc) $(vulkan_inc) $(flags) -c src/vktexture.c -o obj/vktexture.o

obj/vkapp.o: src/vkapp.c
	$(cc) $(vulkan_inc) $(flags) -c src/vkapp.c -o obj/vkapp.o

del:
	del $(exe)

$(exe):
	$(cc) obj/main.o obj/logger.o obj/vkboilerplate.o obj/vkdebug.o obj/win32.o obj/vkcore.o obj/fileio.o obj/vkdoodad.o obj/vkpool.o obj/vkmemory.o obj/vkbufferalloc.o obj/bmploader.o obj/vktexture.o obj/vkapp.o -o $@ $(libs)
