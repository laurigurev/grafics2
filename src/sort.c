#include "grafics2.h"

void bubble_sorts(short* arr, uint64_t size)
{
	if (size == 0) { return; }
	
	for (uint64_t i = 0; i < size - 1; i++) {
		short swapped;
		for (uint64_t j = 0; j < size - i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				short tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
				swapped = 1;
			}
		}

		if (swapped == 0) { break; }
	}
}

void bubble_sortui(uint32_t* arr, uint64_t size)
{
	if (size == 0) { return; }
	
	for (uint64_t i = 0; i < size - 1; i++) {
		uint32_t swapped;
		for (uint64_t j = 0; j < size - i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				uint32_t tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
				swapped = 1;
			}
		}

		if (swapped == 0) { break; }
	}
}

void bubble_sortf(s32* arr, uint64_t size)
	{
	if (size == 0) { return; }
	
	for (uint64_t i = 0; i < size - 1; i++) {
		s32 swapped;
		for (uint64_t j = 0; j < size - i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				s32 tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
				swapped = 1;
			}
		}

		if (swapped == 0) { break; }
	}
}

i32 cmp_floats_callback(const void* f0, const void* f1)
{
	// THIS IS A CALLBACK FUNCTION FOR QSORT
	return (i32) (*(s32*)f0 - *(s32*)f1);
}
