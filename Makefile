CXX=g++
CXXFLAGS=-Wall -c
LDFLAGS=
HEADERS=ssd.h


RAID0_SOURCES=ssd_address.cpp ssd_block.cpp ssd_bm.cpp ssd_bus.cpp \
		ssd_channel.cpp ssd_controller.cpp ssd_die.cpp \
		ssd_event.cpp ssd_config.cpp ssd_ftlparent.cpp ssd_gc.cpp \
		ssd_package.cpp ssd_page.cpp ssd_plane.cpp ssd_ram.cpp \
		ssd_stats.cpp ssd_ssd.cpp ssd_wl.cpp ssd_raidssd.cpp\
		FTLs/dftl_parent.cpp FTLs/dftl_ftl.cpp FTLs/bdftl_ftl.cpp \
		FTLs/fast_ftl.cpp FTLs/page_ftl.cpp FTLs/bast_ftl.cpp \
		run_RAID0.cpp 
RAID0_OBJECTS=$(RAID0_SOURCES:.cpp=.o)


RAID5_SOURCES=ssd_address.cpp ssd_block.cpp ssd_bm.cpp ssd_bus.cpp \
		ssd_channel.cpp ssd_controller.cpp ssd_die.cpp \
		ssd_event.cpp ssd_config.cpp ssd_ftlparent.cpp ssd_gc.cpp \
		ssd_package.cpp ssd_page.cpp ssd_plane.cpp ssd_ram.cpp \
		ssd_stats.cpp ssd_ssd.cpp ssd_wl.cpp ssd_raidssd.cpp\
		FTLs/dftl_parent.cpp FTLs/dftl_ftl.cpp FTLs/bdftl_ftl.cpp \
		FTLs/fast_ftl.cpp FTLs/page_ftl.cpp FTLs/bast_ftl.cpp \
		ssd_stripe.cpp run_RAID5.cpp 
RAID5_OBJECTS=$(RAID5_SOURCES:.cpp=.o)


RAID6_SOURCES=ssd_address.cpp ssd_block.cpp ssd_bm.cpp ssd_bus.cpp \
		ssd_channel.cpp ssd_controller.cpp ssd_die.cpp \
		ssd_event.cpp ssd_config.cpp ssd_ftlparent.cpp ssd_gc.cpp \
		ssd_package.cpp ssd_page.cpp ssd_plane.cpp ssd_ram.cpp \
		ssd_stats.cpp ssd_ssd.cpp ssd_wl.cpp ssd_raidssd.cpp\
		FTLs/dftl_parent.cpp FTLs/dftl_ftl.cpp FTLs/bdftl_ftl.cpp \
		FTLs/fast_ftl.cpp FTLs/page_ftl.cpp FTLs/bast_ftl.cpp \
		ssd_stripe.cpp run_RAID6.cpp 
RAID6_OBJECTS=$(RAID6_SOURCES:.cpp=.o)


raid0: $(HEADERS) $(RAID0_OBJECTS)
	$(CXX) $(LDFLAGS) $(RAID0_OBJECTS) -o $@
	
raid5: $(HEADERS) $(RAID5_OBJECTS)
	$(CXX) $(LDFLAGS) $(RAID5_OBJECTS) -o $@


raid6: $(HEADERS) $(RAID6_OBJECTS)
	$(CXX) $(LDFLAGS) $(RAID6_OBJECTS) -o $@
	
	
.cpp.o: $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	-rm -rf *.o flashsim_dftl
