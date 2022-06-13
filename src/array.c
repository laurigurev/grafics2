#include "grafics2.h"

#define ARRAY_ALLOC_SIZE 16

void arr_init(array* arr, uint64_t stride)
{
	arr->stride = stride;
	arr->alloc_size = ARRAY_ALLOC_SIZE;
	arr->size = 0;
	arr->data = calloc(arr->alloc_size, arr->stride);
}

void arr_add(array* arr, void* src)
{
	memcpy(arr->data + (arr->stride * arr->size), src, arr->stride);
	arr->size++;
	if (arr->size == arr->alloc_size) {
		arr->alloc_size *= 2;
		arr->data = realloc(arr->data, arr->stride * arr->alloc_size);
	}
}

void arr_pop(array* arr)
{
	if (arr->size != 0) {
		// this is memory safe, might be unnecessary
		memset(arr->data + (arr->stride * arr->size), 0, arr->stride);
		arr->size--;
	}
}

void arr_clean(array* arr)
{
	if (arr->size != 0) {
		// this is memory safe, might be unnecessary
		memset(arr->data + (arr->stride * arr->size), 0, arr->stride * arr->size);
		arr->size = 0;
	}
}

void arr_free(array* arr)
{
	arr->stride = 0;
	arr->alloc_size = 0;
	arr->size = 0;
	free(arr->data);
}

uint64_t arr_sizeof(array* arr)
{
	return (uint64_t) arr->stride * (uint64_t) arr->size;
}

void* arr_get(array* arr, uint32_t index)
{
	if (index < arr->size) {
		return arr->data + (arr->stride * index);
	}
	return NULL;
}
