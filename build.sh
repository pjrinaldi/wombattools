#!/bin/bash

echo "Builing wombatimager"

gcc -O3 -o wombatimager wombatimager.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -ludev -lzstd

echo "Building wombatinfo"

gcc -O3 -o wombatinfo wombatinfo.c

echo "Building wombatverify"
	
gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

echo "Building wombatfuse"

gcc -O3 -o wombatmount wombatfuse.c -I/usr/include/fuse3 -lfuse3 -lpthread -lzstd
