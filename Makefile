cc := gcc
exe := a.exe
vulkan_inc := -IC:/VulkanSDK/1.3.204.1/Include/vulkan
vulkan_lib := -LC:/VulkanSDK/1.3.204.1/Lib -lvulkan-1
win32_lib := -lgdi32 -luser32 -lkernel32 -lcomctl32 -lm -mwindows
libs := $(vulkan_lib) $(win32_lib)
flags := -g -Wall -O0 -DVK_USE_PLATFORM_WIN32_KHR

all: main.o logger.o vkboilerplate.o vkdebug.o win32.o vkcore.o fileio.o vkdoodad.o vkapp.o del $(exe)

main.o:
	$(cc) $(vulkan_inc) $(flags) -c src/main.c -o obj/main.o

logger.o:
	$(cc) $(vulkan_inc) $(flags) -c src/logger.c -o obj/logger.o

vkboilerplate.o:
	$(cc) $(vulkan_inc) $(flags) -c src/vkboilerplate.c -o obj/vkboilerplate.o

vkdebug.o:
	$(cc) $(vulkan_inc) $(flags) -c src/vkdebug.c -o obj/vkdebug.o

win32.o:
	$(cc) $(vulkan_inc) $(flags) -c src/win32.c -o obj/win32.o

vkcore.o:
	$(cc) $(vulkan_inc) $(flags) -c src/vkcore.c -o obj/vkcore.o

fileio.o:
	$(cc) $(vulkan_inc) $(flags) -c src/fileio.c -o obj/fileio.o

vkdoodad.o:
	$(cc) $(vulkan_inc) $(flags) -c src/vkdoodad.c -o obj/vkdoodad.o

vkapp.o:
	$(cc) $(vulkan_inc) $(flags) -c src/vkapp.c -o obj/vkapp.o

del:
	del $(exe)

$(exe):
	$(cc) obj/main.o obj/logger.o obj/vkboilerplate.o obj/vkdebug.o obj/win32.o obj/vkcore.o obj/fileio.o obj/vkdoodad.o obj/vkapp.o -o $@ $(libs)
