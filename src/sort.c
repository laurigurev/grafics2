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
