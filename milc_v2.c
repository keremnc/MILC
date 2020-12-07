#include <stdio.h>

#include "milc.h"

#define BLOCK_SIZE 160

typedef struct metadata_block {
	uint32_t start_value;
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

size_t compress_v2(unsigned char** memptr, uint32_t n, uint32_t* sorted_array) {
	uint32_t* split_points = (uint32_t*) malloc((n + 1) * sizeof(uint32_t));
	bzero(split_points, (n + 1) * sizeof(uint32_t));

	uint32_t bits_required = dynamic_partition(n, sorted_array, split_points);

	int num_meta_blocks = 0;
	uint32_t cur_split = n;
	while (cur_split > 0) {
		cur_split = split_points[cur_split];
		num_meta_blocks++;
	}

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

		metadata_block block = {
			.start_value 	= skip_elem,
			.num_elements	= end_idx - (start_idx + 1),
			.num_bits		= num_bits,
			.offset 		= cur_uint * 32 + cur_bit 
		};
		metadata_blocks[meta_block_idx++] = block;

		for (int i = start_idx + 1; i < end_idx; i++) {
			uint32_t elem = sorted_array[i] - skip_elem;
			if (cur_bit + num_bits <= 32) {
				data_blocks[cur_uint] += elem << cur_bit;
			} else {
				data_blocks[cur_uint] |= elem << cur_bit;
				data_blocks[cur_uint + 1] |= elem >> (32 - cur_bit);

			}
			cur_bit += num_bits;
			if (cur_bit >= 32) {
				cur_bit %= 32;
				cur_uint++;
			}
		}
		end_idx = start_idx;
		start_idx = split_points[start_idx];
	}

	int data_block_bits = 0;
	for (int i = 0; i < meta_block_idx; i++) {
		data_block_bits += (metadata_blocks[i].num_bits * metadata_blocks[i].num_elements);
	}
	int total_size = meta_block_idx * 10 + sizeof(uint32_t) + sizeof(uint32_t) * (int) ceil(data_block_bits / 32.0);
	unsigned char* buf = (unsigned char*) malloc(total_size);

	unsigned char* head = buf;
	memcpy(head, &meta_block_idx, sizeof(uint32_t));
	head += sizeof(uint32_t);

	for (int i = meta_block_idx - 1; i >= 0; i--) {
		memcpy(head, &metadata_blocks[i], sizeof(metadata_block));
		head += sizeof(metadata_block);
	}

	int uint_write_head = cur_uint * 32 + cur_bit;
	memcpy(head, data_blocks, sizeof(uint32_t) * (int) ceil(uint_write_head / 32.0));

	*memptr = buf;
	free(split_points);
	free(metadata_blocks);
	free(data_blocks);
	return total_size;

}

static bool data_block_search(uint32_t num_bits, uint32_t* data_start, uint32_t start_offset, uint32_t num_elements, uint32_t key) {
	uint32_t mask = 0;
	for (int i = 0; i < num_bits; i++) {
		mask = (mask << 1) | 1;
	}

	int l = 0, r = num_elements - 1;
	while (l <= r) {
		int idx = l + (r - l) / 2;
		uint32_t bit_offset = start_offset + (idx * num_bits);
		uint32_t read_idx = bit_offset / 32;
		uint32_t read_offset = bit_offset % 32;

		uint32_t result;
		if (read_offset + num_bits <= 32) {
			result = (data_start[read_idx] >> read_offset) & mask;
		} else {
			result = ((data_start[read_idx] >> read_offset) & mask) | ((data_start[read_idx + 1] << (32 - read_offset)) & mask);
		}
		if (result == key) return true;
		if (result < key) l = idx + 1;
		else r = idx - 1;
	}
	return false;
}

bool contains_v2(unsigned char* compressed, uint32_t value) {
	uint32_t key = value;
	uint32_t num_blocks = (uint32_t) compressed[0];
	metadata_block* blocks = (metadata_block*) (compressed + sizeof(uint32_t));

	int l = 0, r = num_blocks - 1;
	while (l <= r) {
		int mid = l + (r - l) / 2;
		uint32_t result = blocks[mid].start_value;
		if (result == key) return true;
		if (result < key) l = mid + 1;
		else r = mid - 1;
	}
	if (r < 0 || r >= num_blocks) return false;

	metadata_block target = blocks[r];
	key -= target.start_value;

	uint32_t* data_offset = (uint32_t*) (compressed + sizeof(uint32_t) + (num_blocks * sizeof(metadata_block)) + (target.offset / 8));
	uint32_t bit_offset = target.offset % 8;

	return data_block_search(target.num_bits, data_offset, bit_offset, target.num_elements, key);
}
