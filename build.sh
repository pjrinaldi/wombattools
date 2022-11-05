#!/bin/bash

#echo "Builing wombatimager"

#gcc -O3 -o wombatimager wombatimager.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -ludev -lzstd

echo "Building wombatinfo"

gcc -O3 -o wombatinfo wombatinfo.c

echo "Install wombatinfo (root)"

sudo cp wombatinfo /usr/local/bin/

echo "Building wombatverify"
	
gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

echo "Install wombatverify (root)"

sudo cp wombatverify /usr/local/bin/

echo "Building wombatmount"

gcc -O3 -o wombatmount wombatfuse.c -I/usr/include/fuse3 -lfuse3 -lpthread -lzstd

echo "Install wombatmount (root)"

sudo cp wombatmount /usr/local/bin/

echo "Building libblake3.so"

gcc -shared -O3 -o libblake3.so blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S

echo "Install libblake3.so to /usr/local/lib (root)"

sudo cp libblake3.so /usr/local/lib/

echo "reload library config (root)"

sudo ldconfig

#echo "Building libblake3.a for c++ programs"

#gcc -c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S

#echo "Linking together to a static library"

#ar -rcs libblake3.a blake3_avx2_x86-64_unix.o blake3_avx512_x86-64_unix.o blake3_dispatch.o blake3.o blake3_portable.o blake3_sse2_x86-64_unix.o blake3_sse41_x86-64_unix.o

echo "Building wombatimager"

g++ -O3 -o wombatimager wombatimager.cpp -lzstd -ludev -lpthread -L. -lblake3
#g++ -O3 -static -o wombatimager wombatimager.cpp -lzstd -ludev -lpthread -L. -lblake3

echo "Install wombatimager (root)"

sudo cp wombatimager /usr/local/bin/

echo "Building wombatrestore"

g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

echo "Install wombatrestore (root)"

sudo cp wombatrestore /usr/local/bin/

# Not ready for use yet...
#echo "Building wombatlogical"

#g++ -O3 -o wombatlogical wombatlogical.cpp -ltar -lzstd -lpthread -L. -lblake3

