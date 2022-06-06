#include "grafics2.h"

char* file_read(const char* path, uint32_t* file_size)
{
	FILE* file;
	file = fopen(path, "rb");
	
	if (!file)
	{
		fprintf(stderr, "unable to open file %s", path);
		return NULL;
	}
	
	*file_size = 0;
	fseek(file, 0, SEEK_END);
	*file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	char* buffer = (char*) malloc(*file_size);
	if (!buffer)
	{
		fprintf(stderr, "unable to allocate buffer %s", path);
		fclose(file);
		return NULL;
	}
	
	fread(buffer, *file_size, 1, file);
	fclose(file);
	
	return buffer;
}

void file_free(void* buffer)
{
	if (buffer)
	{
		free(buffer);
	}
}
