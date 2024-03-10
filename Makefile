blake3:
	cd blake3
	make blake3
	cd ..

mkwalafus:
	g++ -O3 -o mkwalafus mkwalafus.cpp walafus/filesystem.cpp walafus/wltg_packer.cpp walafus/wltg_internal_writers.cpp -lzstd

rdwalafus:
	g++ -O3 -o rdwalafus rdwalafus.cpp walafus/filesystem.cpp walafus/wltg_reader.cpp walafus/wltg_packer.cpp walafus/wltg_internal_readers.cpp walafus/wltg_internal_writers.cpp -lzstd

verifywalafus:
	g++ -O3 -o verifywalafus verifywalafus.cpp walafus/filesystem.cpp walafus/wltg_reader.cpp walafus/wltg_packer.cpp walafus/wltg_internal_readers.cpp walafus/wltg_internal_writers.cpp -lzstd blake3/libblake3.a

WALAFUS_OBJS = walafus/filesystem.cpp walafus/wltg_reader.cpp walafus/wltg_packer.cpp walafus/wltg_internal_readers.cpp walafus/wltg_internal_writers.cpp

wombatimager: $(WALAFUS_OBJS)
	g++ -O3 -o wombatimager $(WALAFUS_OBJS) wombatimager.cpp -ludev -lpthread -lzstd blake3/libblake3.a

wombatread: $(WALAFUS_OBJS)
	g++ -O3 -o wombatreader $(WALAFUS_OBJS) wombatreader.cpp -lzstd

#b3hasher:
#	g++ -O3 -o b3hasher b3hasher.cpp -lpthread blake3/libblake3.a

wombatinfo: $(WALAFUS_OBJS)
	g++ -O3 -o wombatinfo $(WALAFUS_OBJS) wombatinfo.cpp -lzstd

wombatlog: $(WALAFUS_OBJS)
	g++ -O3 -o wombatlog $(WALAFUS_OBJS) wombatlog.cpp -lzstd

wombatverify: $(WALAFUS_OBJS)
	g++ -O3 -o wombatverify $(WALAFUS_OBJS) wombatverify.cpp -lzstd blake3/libblake3.a

#wombatverify:
#	gcc -O3 -o wombatverify wombatverify.c blake3.c blake3_dispatch.c blake3_portable.c blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S -lzstd

wombatrestore:
	g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

#wombatlogical:
