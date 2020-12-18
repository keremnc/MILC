#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <math.h>
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "milc.h"

/*
	Binary search on uncompressed data
*/
bool contains_uncompressed(uint32_t n, uint32_t* uncompressed, uint32_t value) {
	int l = 0, r = n - 1;
	while (l <= r) {
		int mid = l + (r - l) / 2;
		uint32_t result = uncompressed[mid];
		if (result == value) return true;
		if (result < value) l = mid + 1;
		else r = mid - 1;
	}
	return false;
}

int validate(uint32_t n, uint32_t* sorted, uint32_t max_num)
{
	printf("Processing %d elements\n", n);
	unsigned char *v1, *v2, *v3, *v4, *v5, *simdized;
	compress_v1(&v1, n, sorted);
	compress_v2(&v2, n, sorted);
	compress_v3(&v3, n, sorted);
	compress_v4(&v4, n, sorted);
	compress_v5(&v5, n, sorted);
	simdize(&simdized, n, sorted);

	double v1_time = 0;
	double v2_time = 0;
	double v3_time = 0;
	double v4_time = 0;
	double v5_time = 0;
	double un_time = 0;
	double simd_time = 0;

	clock_t start, end;
	for (int i = 0; i < max_num; i += 1) {
		if (i % 10000000 == 0) printf("i=%d\n", i);
		start = clock();
		bool v1_result = contains_v1(v1, i);
		end = clock();
		v1_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		start = clock();
		bool v2_result = contains_v2(v2, i);
		end = clock();
		v2_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		start = clock();
		bool v3_result = contains_v3(v3, i);
		end = clock();
		v3_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		start = clock();
		bool v4_result = contains_v4(v4, i);
		end = clock();
		v4_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		start = clock();		
		bool v5_result = contains_v5(v5, i);
		end = clock();
		v5_time += ((double) (end - start)) / CLOCKS_PER_SEC;		

		start = clock();
		bool gt = contains_uncompressed(n, sorted, i);
		end = clock();
		un_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		start = clock();		
		bool simd = contains_simdized(n, simdized, i);
		end = clock();
		simd_time += ((double) (end - start)) / CLOCKS_PER_SEC;

		if (v1_result != v2_result || v2_result != v3_result || v3_result != v4_result || v4_result != v5_result || v5_result != gt || gt != simd) {
			printf("Discrepancy for %d\n", i);
			printf("v1=%d\n", v1_result);
			printf("v2=%d\n", v2_result);
			printf("v3=%d\n", v3_result);
			printf("v4=%d\n", v4_result);
			printf("v5=%d\n", v5_result);
			printf("simdized=%d\n", simd);
			printf("uncompressed=%d\n", gt);
			exit(1);
		}
	}

	printf("Success\n");
	printf("v1 time: %f seconds\n", v1_time);
	printf("v2 time: %f seconds\n", v2_time);
	printf("v3 time: %f seconds\n", v3_time);
	printf("v4 time: %f seconds\n", v4_time);
	printf("v5 time: %f seconds\n", v5_time);
	printf("uncompressed time: %f seconds\n", un_time);
	printf("simdized time: %f seconds\n", simd_time);
	free(v1);
	free(v2);
	free(v3);
	free(v4);	
	free(simdized);	
	return 0;
}

uint32_t* generate_list(uint32_t n) {
	uint32_t* list = (uint32_t*) malloc(n * sizeof(uint32_t*));
	double scale = 1;
	double shape = 1;

	uint32_t cur = 1;
	for (uint32_t i = 0; i < n; i++) {
		double r = ((double) rand() / (RAND_MAX));
		cur = (uint32_t) ceil(cur + (scale / pow(r, 1 / shape)));
		list[i] = cur;
	}
	return list;
}

int main() {
	srand(time(NULL));
	for (uint32_t p = 12; p < 26; p++) {
		uint32_t n = (uint32_t) pow(2, p);
		uint32_t* list = generate_list(n);
		uint32_t max_num = list[n - 1];
		printf("===== VALIDATING FOR p=%d =====\n", p);
		validate(n, list, max_num);
		free(list);
	}
}
