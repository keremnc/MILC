#include <stdio.h>
#include <immintrin.h>

#include "milc.h"

#define BLOCK_SIZE 160
#define TREE_CHILDREN 17
#define COMPRESSION_PAD 60

typedef struct metadata_block {
	uint32_t offset;
	uint8_t num_elements;
	uint8_t num_bits;
} __attribute__((packed)) metadata_block;

static uint32_t cost(int j, int i, uint32_t* sorted_array) {
	return (int) ceil(log2(sorted_array[i - 1] - sorted_array[j - 1] + 1)) * (i -j) + 80;
}

static int dynamic_partition(uint32_t n, uint32_t* sorted_array, uint32_t* split_points) {
	uint32_t* E = (uint32_t*) malloc((n + 1) * sizeof(uint32_t));
	E[1] = 80;
	for (int i = 1; i <= n; i++) {
		uint32_t min_score = UINT32_MAX;
		uint32_t split_point = -1;

		for (int j = (i - BLOCK_SIZE) > 0 ? (i - BLOCK_SIZE) : 0; j < i; j++) {
			uint32_t score = E[j] + cost(j + 1, i, sorted_array);
			if (score < min_score) {
				min_score = score;
				split_point = j;
			}
		}
		E[i] = min_score;
		split_points[i] = split_point;
	}
	uint32_t total_cost = E[n];
	free(E);
	return total_cost;
}

static int max_sub_block_cost(uint32_t n, uint32_t* block, int k) {
	uint32_t s = (int) (floor(n / k));
	int max_sub_block_cost = 0;
	for (int i = 0; i < k; i++) {
		uint32_t sb_start = i * s;
		uint32_t sb_end = ((i + 1) * s) - 1;
		if (i == k - 1) {
			sb_end = n - 1;
		}
		int cost = (int) ceil(log2(block[sb_end] - block[sb_start] + 1));
		if (cost > max_sub_block_cost) {
			max_sub_block_cost = cost;
		}
	}
	return max_sub_block_cost;
}

static int mini_skip_cost(uint32_t n, uint32_t* block, int k, uint32_t skip_elem) {
	return (max_sub_block_cost(n, block, k) * (n - k)) 
			+ ((int) ceil(log2(block[n - 1] - skip_elem + 1)) * k) 
			+ 16;
}

static uint8_t optimal_mini_skips(uint32_t n, uint32_t* block, uint32_t skip_elem) {
	if (n == 0) return 0;
	int min_k = 1;
	int min_cost = mini_skip_cost(n, block, min_k, skip_elem);
	for (int k = 2; k < n / 4; k++) {
		int k_cost = mini_skip_cost(n, block, k, skip_elem);
		if (k_cost < min_cost) {
			min_cost = k_cost;
			min_k = k;
		}
	}
	return min_k;
}

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


size_t compress_v5(unsigned char** memptr, uint32_t n, uint32_t* sorted_array) {
	uint32_t* split_points = (uint32_t*) malloc((n + 1) * sizeof(uint32_t));
	bzero(split_points, (n + 1) * sizeof(uint32_t));

	uint32_t bits_required = dynamic_partition(n, sorted_array, split_points) + (int) ceil(n / 2.0);

	int num_meta_blocks = 0;
	uint32_t cur_split = n;
	while (cur_split > 0) {
		cur_split = split_points[cur_split];
		num_meta_blocks++;
	}

	uint32_t* meta_skip_ptrs = (uint32_t*) malloc(num_meta_blocks * sizeof(metadata_block));
	metadata_block* metadata_blocks = (metadata_block*) malloc(num_meta_blocks * sizeof(metadata_block));
	int meta_block_idx = 0;

	uint32_t* data_blocks = (uint32_t*) malloc((int) ceil(bits_required / 8.0));
	bzero(data_blocks, (int) ceil(bits_required / 8.0));

	uint32_t cur_uint = 0;
	uint32_t cur_bit = 0;

	int end_idx = n;
	int start_idx = split_points[n];

	while (end_idx > start_idx) {
		uint32_t skip_elem = sorted_array[start_idx];
		uint32_t last_elem = sorted_array[end_idx - 1];
		uint8_t num_bits = (int) ceil(log2(last_elem - skip_elem + 1));
		uint32_t num_elems = end_idx - (start_idx + 1);

		metadata_block block = {
			.num_elements	= num_elems,
			.num_bits		= num_bits,
			.offset 		= cur_uint * 32 + cur_bit 
		};
		int linear_meta_idx = num_meta_blocks - ++meta_block_idx;
		int tree_meta_idx = g(linear_meta_idx + 1, num_meta_blocks) - 1;
		metadata_blocks[tree_meta_idx] = block;
		meta_skip_ptrs[tree_meta_idx] = skip_elem;

		uint8_t num_mini_skips = 0;		
		uint8_t num_sub_block_bits = 0;

		if (num_elems > 0) {
			num_mini_skips = optimal_mini_skips(num_elems, &sorted_array[start_idx + 1], skip_elem);
			num_sub_block_bits = max_sub_block_cost(num_elems, &sorted_array[start_idx + 1], num_mini_skips);
		}

		if (cur_bit + 8 <= 32) {
			data_blocks[cur_uint] |= (num_sub_block_bits & 0xFF) << cur_bit;
		} else {
			data_blocks[cur_uint] |= (num_sub_block_bits & 0xFF) << cur_bit;
			data_blocks[cur_uint + 1] |= (num_sub_block_bits & 0xFF) >> (32 - cur_bit);
		}
		cur_bit += 8;
		if (cur_bit >= 32) {
			cur_bit %= 32;
			cur_uint++;
		}		
		if (cur_bit + 8 <= 32) {
			data_blocks[cur_uint] |= (num_mini_skips & 0xFF) << cur_bit;
		} else {
			data_blocks[cur_uint] |= (num_mini_skips & 0xFF) << cur_bit;
			data_blocks[cur_uint + 1] |= (num_mini_skips & 0xFF) >> (32 - cur_bit);
		}
		cur_bit += 8;
		if (cur_bit >= 32) {
			cur_bit %= 32;
			cur_uint++;
		}

		if (num_mini_skips > 0) {
			uint32_t s = (int) (floor(num_elems / num_mini_skips));
			for (int i = 0; i < num_mini_skips; i++) {
				int mskp_start_idx = (start_idx + 1) + (i * s);
				uint32_t mskp_elem = sorted_array[mskp_start_idx] - skip_elem;
				if (cur_bit + num_bits <= 32) {
					data_blocks[cur_uint] |= mskp_elem << cur_bit;
				} else {
					data_blocks[cur_uint] |= mskp_elem << cur_bit;
					data_blocks[cur_uint + 1] |= mskp_elem >> (32 - cur_bit);
				}
				cur_bit += num_bits;
				if (cur_bit >= 32) {
					cur_bit %= 32;
					cur_uint++;
				}
			}
			for (int i = 0; i < num_mini_skips; i++) {
				int mskp_start_idx = (start_idx + 1) + (i * s);
				uint32_t mskp_elem = sorted_array[mskp_start_idx] - skip_elem;		
				int sb_start_idx = (start_idx + 1) + ((i * s) + 1);
				int sb_end_idx = (start_idx + 1) + (((i + 1) * s) - 1);
				if (i == num_mini_skips - 1) {
					sb_end_idx = end_idx - 1;
				}
				for (int j = sb_start_idx; j <= sb_end_idx; j++) {
					uint32_t sb_elem = sorted_array[j] - mskp_elem - skip_elem;
					if (cur_bit + num_sub_block_bits <= 32) {
						data_blocks[cur_uint] |= sb_elem << cur_bit;
					} else {
						data_blocks[cur_uint] |= sb_elem << cur_bit;
						data_blocks[cur_uint + 1] |= sb_elem >> (32 - cur_bit);
					}
					cur_bit += num_sub_block_bits;
					if (cur_bit >= 32) {
						cur_bit %= 32;
						cur_uint++;
					}
				}
			}
		}
		end_idx = start_idx;
		start_idx = split_points[start_idx];
	}

	int uint_write_head = cur_uint * 32 + cur_bit;

	int total_size = COMPRESSION_PAD + meta_block_idx * (6 + sizeof(uint32_t)) + sizeof(uint32_t) + sizeof(uint32_t) * (int) ceil(uint_write_head / 32.0);
	void* buf;
	posix_memalign(&buf, 64, total_size);

	unsigned char* head = buf + COMPRESSION_PAD;
	memcpy(head, &meta_block_idx, sizeof(uint32_t));
	head += sizeof(uint32_t);

	for (int i = 0; i < meta_block_idx; i++) {
		memcpy(head, &meta_skip_ptrs[i], sizeof(uint32_t));
		head += sizeof(uint32_t);
	}

	for (int i = 0; i < meta_block_idx; i++) {
		memcpy(head, &metadata_blocks[i], sizeof(metadata_block));
		head += sizeof(metadata_block);
	}

	memcpy(head, data_blocks, sizeof(uint32_t) * (int) ceil(uint_write_head / 32.0));

	*memptr = buf;
	free(split_points);
	free(metadata_blocks);
	free(data_blocks);
	free(meta_skip_ptrs);
	return total_size;

}

static bool data_block_search(uint32_t num_bits, uint32_t* data_start, uint32_t start_offset, uint32_t num_elements, uint32_t key) {
	int header_idx = start_offset / 32;
	int header_offset = start_offset % 32;

	uint16_t block_header 		= _bextr_u64(*((uint64_t*) (data_start + header_idx)), header_offset, 16);
	uint8_t num_sub_block_bits 	= _bextr_u32(block_header, 0, 8);
	uint8_t num_mini_skips 		= _bextr_u32(block_header, 8, 8);

	int mskps_start_offset = start_offset + 16;
	int l = 0, r = num_mini_skips - 1;
	while (l <= r) {
		int mid = l + (r - l) / 2;
		uint32_t bit_offset = mskps_start_offset + (mid * num_bits);
		uint32_t read_idx = bit_offset / 32;
		uint32_t read_offset = bit_offset % 32;

		uint32_t result = _bextr_u64(*((uint64_t*) (data_start + read_idx)), read_offset, num_bits);

		if (result == key) return true;
		if (result < key) l = mid + 1;
		else r = mid - 1;
	}
	if (r < 0 || r >= num_mini_skips) return false;
	int mskp_offset = mskps_start_offset + (r * num_bits);
	uint32_t read_idx = mskp_offset / 32;
	uint32_t read_offset = mskp_offset % 32;
	uint32_t mskp_elem = _bextr_u64(*((uint64_t*) (data_start + read_idx)), read_offset, num_bits);

	uint32_t s = (int) (floor(num_elements / num_mini_skips));
	uint32_t sub_block_key = key - mskp_elem;

	l = r * (s - 1);
	if (r == num_mini_skips - 1) {
		r = num_elements - num_mini_skips - 1;
	} else {
		r = (r + 1) * (s - 1) - 1;
	}
	int elem_start_offset = mskps_start_offset + (num_mini_skips * num_bits);
	while (l <= r) {
		int idx = l + (r - l) / 2;
		uint32_t bit_offset = elem_start_offset + (idx * num_sub_block_bits);
		uint32_t read_idx = bit_offset / 32;
		uint32_t read_offset = bit_offset % 32;

		uint32_t result = _bextr_u64(*((uint64_t*) (data_start + read_idx)), read_offset, num_sub_block_bits);
		if (result == sub_block_key) return true;
		if (result < sub_block_key) l = idx + 1;
		else r = idx - 1;
	}
	return false;
}

bool contains_v5(unsigned char* compressed, uint32_t value) {
	uint32_t key = value;
	uint32_t num_blocks = ((uint32_t*) (compressed + COMPRESSION_PAD))[0];

	uint32_t* meta_skip_ptrs = (uint32_t*) (compressed + COMPRESSION_PAD + sizeof(uint32_t));
	metadata_block* blocks = (metadata_block*) (compressed + COMPRESSION_PAD + ((num_blocks + 1) * sizeof(uint32_t)));

    __m512i max_mask = _mm512_set1_epi32(UINT32_MAX);
	__m512i search = _mm512_set1_epi32(key);

	int l = 1;
	int cur = 0;

	while (l <= num_blocks) {
		__mmask16 mask = UINT16_MAX;
		if (l + TREE_CHILDREN > num_blocks) {
			mask << (num_blocks - TREE_CHILDREN);
		}

		__m512i keys = _mm512_mask_loadu_epi32(max_mask, mask, meta_skip_ptrs + l - 1);


		if (_mm512_cmpeq_epu32_mask(keys, search) > 0) {
			return true;
		}

		__mmask16 cmp = _mm512_cmpgt_epu32_mask(search, keys);

		int part = 0;
		if (cmp > 0) {
			int bits_set = _mm_popcnt_u32(cmp);
			cur = l + bits_set - 1;
			part = bits_set;
		}
		l = (l * TREE_CHILDREN) + (part * (TREE_CHILDREN - 1));
	}

	cur--;
	if (cur < 0 || cur >= num_blocks) return false;

	metadata_block target = blocks[cur];
	key -= meta_skip_ptrs[cur];

	uint32_t* data_offset = (uint32_t*) (COMPRESSION_PAD + compressed + sizeof(uint32_t) + (num_blocks * (sizeof(uint32_t) + sizeof(metadata_block))));

	return data_block_search(target.num_bits, data_offset, target.offset, target.num_elements, key);
}
