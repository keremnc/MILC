#pragma once

/*
	V1: Static partitioning
*/
bool contains_v1(unsigned char* compressed, uint32_t value);
size_t compress_v1(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);

/*
	V2: Dynamic partitioning
*/
bool contains_v2(unsigned char* compressed, uint32_t value);
size_t compress_v2(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);

/*
	V3: In-block compression
*/
bool contains_v3(unsigned char* compressed, uint32_t value);
size_t compress_v3(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);

/*
	V4: Cache-conscious compression
*/
bool contains_v4(unsigned char* compressed, uint32_t value);
size_t compress_v4(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);

/*
	V4: SIMD Acceleration
*/
bool contains_v5(unsigned char* compressed, uint32_t value);
size_t compress_v5(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);

/*
	SIMD-ized K-ary search
*/
bool contains_simdized(uint32_t n, unsigned char* compressed, uint32_t value);
size_t simdize(unsigned char** memptr, uint32_t n, uint32_t* sorted_array);
