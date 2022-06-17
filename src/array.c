#include "grafics2.h"

#define ARRAY_ALLOC_SIZE 16

void arr_init(Array* arr, uint64_t stride)
{
	arr->stride = stride;
	arr->alloc_size = ARRAY_ALLOC_SIZE;
	arr->size = 0;
	arr->data = calloc(arr->alloc_size, arr->stride);
}

void arr_add(Array* arr, void* src)
{
	memcpy(arr->data + (arr->stride * arr->size), src, arr->stride);
	arr->size++;
	if (arr->size == arr->alloc_size) {
		arr->alloc_size *= 2;
		arr->data = realloc(arr->data, arr->stride * arr->alloc_size);
	}
}

void arr_remove(Array* arr, uint32_t index)
{
	index = index % arr->size;
	memcpy(arr->data + (index * arr->stride), arr->data + ((index + 1) * arr->stride),
		   (arr->size - index - 1) * arr->stride);
	arr->size--;
}

void arr_duplicates(Array* arr)
{
	int res;
	Array tmp;
	arr_init(&tmp, sizeof(uint32_t));
	for (uint32_t i = 0; i < arr->size; i++) {
		for (uint32_t j = i + 1; j < arr->size; j++) {
			// if (j == i) { j++; continue; }
			res = memcmp(arr->data + (i * arr->stride), arr->data + (j * arr->stride),
						 arr->stride);
			if (res == 0) { arr_add(&tmp, &j); }
		}
	}
	bubble_sortui(tmp.data, tmp.size);
	for (int32_t i = tmp.size - 1; i >= 0; i--) {
		uint32_t index = *((uint32_t*) arr_get(&tmp, i));
		arr_remove(arr, index);
	}
	arr_free(&tmp);
}

void arr_pop(Array* arr)
{
	if (arr->size != 0) {
		// this is memory safe, might be unnecessary
		memset(arr->data + (arr->stride * arr->size), 0, arr->stride);
		arr->size--;
	}
}

void arr_clean(Array* arr)
{
	if (arr->size != 0) {
		// this is memory safe, might be unnecessary
		memset(arr->data + (arr->stride * arr->size), 0, arr->stride * arr->size);
		arr->size = 0;
	}
}

void arr_free(Array* arr)
{
	arr->stride = 0;
	arr->alloc_size = 0;
	arr->size = 0;
	if (arr->data) {
		free(arr->data);
	}
}

uint64_t arr_sizeof(Array* arr)
{
	return (uint64_t) arr->stride * (uint64_t) arr->size;
}

void arr_copy(Array* arr, void* src, uint32_t count)
{
	if (count == 0) { return; }
	if (arr->size + count >= arr->alloc_size) {
		arr->alloc_size *= 2;
		arr->data = realloc(arr->data, arr->stride * arr->alloc_size);
		memcpy(arr->data + (arr->size * arr->stride), src, count * arr->stride);
		arr->size += count;
	}
	else {
		memcpy(arr->data + (arr->size * arr->stride), src, count * arr->stride);
		arr->size += count;
	}
}

void* arr_get(Array* arr, uint32_t index)
{
	index = index % arr->size;
	return arr->data + (arr->stride * index);
}
