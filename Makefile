blake3:
	cd blake3
	make blake3
	cd ..

wombatimager:
	g++ -O3 -o wombatimager wombatimager.cpp -lzstd -ludev -lpthread blake3/libblake3.a

wombatinfo:
	gcc -O3 -o wombatinfo wombatinfo.c

wombatverify:
	gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

wombatmount:
	gcc -O3 -o wombatmount wombatfuse.c -I/usr/include/fuse3 -lfuse3 -lpthread -lzstd

wombatrestore:
	g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

#wombatlogical:
