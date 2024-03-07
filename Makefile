blake3:
	cd blake3
	make blake3
	cd ..

mkwalafus:
	g++ -O3 -o mkwalafus mkwalafus.cpp walafus/filesystem.cpp walafus/wltg_packer.cpp walafus/wltg_internal_writers.cpp -lzstd

rdwalafus:
	g++ -O3 -o rdwalafus rdwalafus.cpp walafus/filesystem.cpp walafus/wltg_reader.cpp walafus/wltg_packer.cpp walafus/wltg_internal_readers.cpp walafus/wltg_internal_writers.cpp -lzstd blake3/libblake3.a

SEEKABLE_OBJS = zstd/zstdseek_compress.c zstd/zstdseek_decompress.c

wombatimager: $(SEEKABLE_OBJS)
	g++ -O3 -o wombatimager $(SEEKABLE_OBJS) wombatimager.cpp -ludev -lpthread zstd/libzstd.a blake3/libblake3.a

b3hasher:
	g++ -O3 -o b3hasher b3hasher.cpp -lpthread blake3/libblake3.a

wombatinfo:
	gcc -O3 -o wombatinfo wombatinfo.c

wombatverify:
	gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

wombatmount: $(SEEKABLE_OBJS)
	gcc -O3 -o wombatmount $(SEEKABLE_OBJS) wombatfuse.c -I/usr/include/fuse3 -lfuse3 -lc -lpthread zstd/libzstd.a

wombatrestore:
	g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

#wombatlogical:
