
lib_dir = .lib
dist_dir = dist
ais_dir = $(lib_dir)/ais/
libais = $(ais_dir)/dist/libmessage.a
aivdm_dir = $(lib_dir)/aivdm
libaivdm = $(aivdm_dir)/dist/libpacket.a

CCFLAGS=-Wall -g -Werror
LDFLAGS=

$(dist_dir)/decode-ais : $(lib_dir)/main.o $(libais) $(libaivdm)
	 mkdir -p $(dist_dir)
	 cc $(LDFLAGS) -o $(dist_dir)/decode-ais $(lib_dir)/main.o $(libais) $(libaivdm)

$(lib_dir)/main.o: main.c $(libais) $(libaivdm)
	 mkdir -p $(lib_dir)
	 cc $(CCFLAGS) -o $(lib_dir)/main.o -c -I $(lib_dir) main.c

$(libaivdm) : ../spec/aivdm.xml
	 mkdir -p $(aivdm_dir)
	 bcompile --encoder -d $(aivdm_dir) ../spec/aivdm.xml
	 make -C $(aivdm_dir)

$(libais) : ../spec/ais.xml
	 mkdir -p $(ais_dir)
	 bcompile -d $(ais_dir) ../spec/ais*.xml
	 make -C $(ais_dir)

clean:
	 rm -rf $(lib_dir)
	 rm -rf $(dist_dir)
