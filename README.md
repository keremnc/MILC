# MILC
This is a rough implementation of the MILC inverted list compression algorithm developed by Dr. Jianguo Wang, et. al.

Each optimization presented in the paper is implemented as a separate version on top of all previous optimizations:
1. **V1**: MILC basic structure with static partitioning
2. **V2**: Dynamic partitioning 
3. **V3**: In-Block Compression
4. **V4**: Cache-Conscious Compression
5. **V5**: Modern CPU-Aware Optimizations


## Building
Simply run `make`. Timing and validation binaries will be built in `./target/bin/timing` and `./target/bin/validation`.
**Note:** You might have to adjust the architecture targeted in `Makefile` if you are not on a microarchitecture compatbile with `-march=skylake-avx512`

## Running
`./target/bin/timing` will measure compression ratio, compression throughput, and membership testing performance for randomly-generated lists from size 2^16 to 2^26.

`./target/bin/validation` will run exhaustive tests to ensure compression and membership testing work appropriately for randomly-generated lists from size 2^12 to 2^26.
