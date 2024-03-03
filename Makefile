blake3:
	cd blake3
	make blake3
	cd ..

SEEKABLE_OBJS = zstd/zstdseek_compress.c zstd/zstdseek_decompress.c

wombatimager: $(SEEKABLE_OBJS)
	g++ -O3 -o wombatimager $(SEEKABLE_OBJS) wombatimager.cpp -ludev -lpthread zstd/libzstd.a blake3/libblake3.a

wombatinfo:
	gcc -O3 -o wombatinfo wombatinfo.c

wombatverify:
	gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

wombatmount:
	gcc -O3 -o wombatmount wombatfuse.c -I/usr/include/fuse3 -lfuse3 -lpthread zstd/libzstd.a

wombatrestore:
	g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

#wombatlogical:
