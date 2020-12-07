#include <stdio.h>

#include "milc.h"

#define BLOCK_SIZE 128

typedef struct metadata_block {
	uint32_t start_value;
	uint32_t offset;
	uint8_t num_elements;
	uint8_t num_bits;
} __attribute__((packed)) metadata_block;

size_t compress_v1(unsigned char** memptr, uint32_t n, uint32_t* sorted_array) {
	uint32_t num_blocks = (uint32_t) ceil((double) n / (BLOCK_SIZE + 1));

	uint32_t* cur_part = (uint32_t*) malloc((BLOCK_SIZE + 1) * sizeof(uint32_t));
	int part_idx = 0;

	metadata_block* metadata_blocks = (metadata_block*) malloc(num_blocks * sizeof(metadata_block));
	int meta_block_idx = 0;

	uint32_t* data_blocks = (uint32_t*) malloc(n * sizeof(uint32_t));
	bzero(data_blocks, n * sizeof(uint32_t));

	uint32_t cur_uint = 0;
	uint32_t cur_bit = 0;

	for (int i = 0; i < n; i++) {
		cur_part[part_idx++] = sorted_array[i];

		if (part_idx == BLOCK_SIZE + 1 || i == n - 1) {
			uint32_t skip_elem = cur_part[0];
			uint32_t last_elem = cur_part[part_idx - 1];
			uint8_t num_bits = (int) ceil(log2(last_elem - skip_elem + 1));

			metadata_block block = { 
				.start_value 	= skip_elem, 
				.num_elements 	= part_idx - 1, 
				.num_bits 		= num_bits, 
				.offset 		= cur_uint * 32 + cur_bit 
			};
			metadata_blocks[meta_block_idx++] = block;

			for (int j = 1; j < part_idx; j++) {
				uint32_t elem = cur_part[j] - skip_elem;

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
			part_idx = 0;
		}
	}

	int data_block_bits = 0;
	for (int i = 0; i < num_blocks; i++) {
		data_block_bits += (metadata_blocks[i].num_bits * metadata_blocks[i].num_elements);
	}
	int total_size = num_blocks * 10 + sizeof(uint32_t) + sizeof(uint32_t) * (int) ceil(data_block_bits / 32.0);
	unsigned char* buf = (unsigned char*) malloc(total_size);
	unsigned char* head = buf;
	memcpy(head, &num_blocks, sizeof(uint32_t));
	head += sizeof(uint32_t);
	memcpy(head, metadata_blocks, meta_block_idx * sizeof(metadata_block));
	head += meta_block_idx * sizeof(metadata_block);

	int uint_write_head = cur_uint * 32 + cur_bit;
	memcpy(head, data_blocks, sizeof(uint32_t) * (int) ceil(uint_write_head / 32.0));

	*memptr = buf;
	free(cur_part);
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

bool contains_v1(unsigned char* compressed, uint32_t value) {
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
