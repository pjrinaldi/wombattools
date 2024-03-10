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

wombatrestore:
	g++ -O3 -o wombatrestore wombatrestore.cpp -lzstd -lpthread -L. -lblake3

#wombatlogical:
