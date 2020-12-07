CC=gcc
CFLAGS=-I.
DEPS = milc.h

%.o: %.c $(DEPS)
	$(CC) -c -march=skylake-avx512 -o $@ $< $(CFLAGS)

milc: milc.o milc_v1.o milc_v2.o milc_v3.o milc_v4.o milc_v5.o simdized.o
	$(CC) -o milc milc.o milc_v1.o milc_v2.o milc_v3.o milc_v4.o milc_v5.o simdized.o -lm -O3

clean:
	rm -f *.o milc 
