# May need to adjust the following two defines:
UL=/usr/local
lfs_cflags=-D_FILE_OFFSET_BITS=64 -DMY_SEEK=fseeko

cc=g++
gdal_cflags=-I$(UL)/include
gdal_ld=-L $(UL)/lib -lgdal
cflags=-Wall -g $(gdal_cflags) $(lfs_cflags)

test: lfstest
	./lfstest 2>/dev/null  
	# printed message should be 01    --Don't forget to delete BIGRASTER

lfstest: lfstest.cc
	$(cc) lfstest.cc -Wall -g $(cflags) -o $@ $(gdal_ld) 
	
tidy:
	rm -f *.o *~
	
clean: tidy
	rm -f lfstest BIGRASTER*
	

