#include <stdio.h>
#include <immintrin.h>

#include <math.h>
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TREE_CHILDREN 17
#define COMPRESSION_PAD 60

static int sign(int x) {
    return (x > 0) - (x < 0);
}

static int d_(int h, int j) {
	return (int) floor(log(j) / log(TREE_CHILDREN));
}

static int o_(int h, int j) {
	return j - (int) pow(TREE_CHILDREN, d_(h, j));
}

static int f_(int h, int j) {
	return ((int) pow(TREE_CHILDREN, h - d_(h, j) - 1)) * ((int) floor((TREE_CHILDREN / (TREE_CHILDREN - 1.0)) * o_(h, j) + 1));
}

static int d(int h, int i) {
	int sum = 0;
	for (int x = 1; x < h; x++) {
		sum += (sign(i % (int) pow(TREE_CHILDREN, h - x)));
	}
	return sum;
}

static int o(int h, int i) {
	return floor(((TREE_CHILDREN - 1.0) / TREE_CHILDREN) * (i / pow(TREE_CHILDREN, h - d(h, i) - 1)));
}

static int f(int h, int i) {
	return (int) pow(TREE_CHILDREN, d(h, i)) + o(h, i);
}

static int g(int i, int n) {
	int H = (int) ceil(log(n + 1)/ log(TREE_CHILDREN));
	if (i <= f_(H, n)) {
		return f(H, i);
	} else {
		return f(H - 1, i - o_(H, n) - 1);
	}
}

size_t simdize(unsigned char** memptr, uint32_t n, uint32_t* sorted_array) {
	int total_size = n * sizeof(uint32_t);
	void* buf;
	posix_memalign(&buf, 64, total_size);
	for (int i = 0; i < n; i++) {
		((uint32_t*) buf)[g(i + 1, n) - 1] = sorted_array[i];
	}
	*memptr = buf;
	return total_size;
}

bool contains_simdized(uint32_t n, unsigned char* compressed, uint32_t value) {
	uint32_t key = value;
	uint32_t* elements = (uint32_t*) compressed;

	__m512i max_mask = _mm512_set1_epi32(UINT32_MAX);
	__m512i search = _mm512_set1_epi32(key);

	int l = 1;

	while (l <= n) {
		__mmask16 mask = UINT16_MAX;
		if (l + TREE_CHILDREN > n) {
			mask << (n - TREE_CHILDREN);
		}
		__m512i keys = _mm512_mask_loadu_epi32(max_mask, mask, elements + l - 1);

		if (_mm512_cmpeq_epu32_mask(keys, search) > 0) {
			return true;
		}

		__mmask16 cmp = _mm512_cmpgt_epu32_mask(search, keys);

		int part = _mm_popcnt_u32(cmp);
		l = (l * TREE_CHILDREN) + (part * (TREE_CHILDREN - 1));
	}
	return false;
}
