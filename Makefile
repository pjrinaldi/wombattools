blake3:
	cd blake3
	make blake3
	cd ..

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

wombatrestore: $(WALAFUS_OBJS)
	g++ -O3 -o wombatrestore $(WALAFUS_OBJS) wombatrestore.cpp -lzstd -lpthread blake3/libblake3.a

wombatlist: $(WALAFUS_OBJS)
	g++ -O3 -o wombatlist $(WALAFUS_OBJS) wombatlist.cpp -lzstd

wombatmount: $(WALAFUS_OBJS)
	g++ -O3 -o wombatmount $(WALAFUS_OBJS) fusepp/Fuse.cpp wombatmount.cpp -I/usr/include/fuse3 -lfuse3 -lzstd -D_FILE_OFFSET_BITS=64

#wombatlogical:
