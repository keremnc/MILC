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

int timings(uint32_t n, uint32_t* sorted, uint32_t max_num) {
	printf("Processing %d elements [%ld bytes]\n", n, n * sizeof(uint32_t));
	unsigned char *v1, *v2, *v3, *v4, *v5, *simdized;
	clock_t start, end;

	start = clock();
	size_t v1_size = compress_v1(&v1, n, sorted);
	end = clock();
	printf("Compressed v1 in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / v1_size);	

	start = clock();
	size_t v2_size = compress_v2(&v2, n, sorted);
	end = clock();
	printf("Compressed v2 in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / v2_size);	

	start = clock();
	size_t v3_size = compress_v3(&v3, n, sorted);
	end = clock();
	printf("Compressed v3 in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / v3_size);	

	start = clock();
	size_t v4_size = compress_v4(&v4, n, sorted);
	end = clock();
	printf("Compressed v4 in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / v4_size);	

	start = clock();
	size_t v5_size = compress_v5(&v5, n, sorted);
	end = clock();
	printf("Compressed v5 in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / v5_size);	

	start = clock();
	size_t simd_size = simdize(&simdized, n, sorted);
	end = clock();
	printf("SIMDized in %f seconds [ratio=%f]\n", ((double) (end - start)) / CLOCKS_PER_SEC, (double) (n * sizeof(uint32_t)) / simd_size);	

	const uint32_t STOP = max_num;
	const uint32_t STEP = 1;

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_v1(v1, i);
	}
	end = clock();
	printf("V1 took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, v1_size);

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_v2(v2, i);
	}
	end = clock();
	printf("V2 took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, v2_size);

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_v3(v3, i);
	}
	end = clock();
	printf("V3 took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, v3_size);

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_v4(v4, i);
	}
	end = clock();
	printf("V4 took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, v4_size);

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_v5(v5, i);
	}
	end = clock();
	printf("V5 took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, v5_size);

	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_uncompressed(n, sorted, i);
	}
	end = clock();
	printf("Uncompressed took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, n * sizeof(uint32_t));
	
	start = clock();
	for (uint32_t i = 0; i < STOP; i += STEP) {
		contains_simdized(n, simdized, i);
	}
	end = clock();
	printf("SIMD-ized took %f seconds [%ld bytes]\n", ((double) (end - start)) / CLOCKS_PER_SEC, simd_size);

	free(v1);
	free(v2);
	free(v3);
	free(v4);
	free(v5);
	free(simdized);
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
	for (uint32_t p = 16; p < 26; p++) {
		uint32_t n = (uint32_t) pow(2, p);
		uint32_t* list = generate_list(n);
		uint32_t max_num = list[n - 1];
		printf("===== TIMINGS FOR p=%d =====\n", p);
		printf("===== Operations=%d =====\n", max_num);
		timings(n, list, max_num);
		free(list);
	}
}
