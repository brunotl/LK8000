#
# Copyright (c) 2013 No Face Press, LLC
# License http://opensource.org/licenses/mit-license.php MIT License
#
# Standalone build: needs a sibling ../civetweb source checkout. The
# TARGET=KOBO KOBO_SDK=y cross-build does NOT use this Makefile -- it
# builds against Buildroot's own civetweb package instead, see
# buildroot/package/lk8000websrv/lk8000websrv.mk.

PROG = embedded_c
SRC = embedded_c.c

TOP = ../civetweb
CIVETWEB_LIB = libcivetweb.a

CFLAGS = -I$(TOP)/include $(COPT) -DUSE_WEBSOCKET -DUSE_IPV6 -DNO_SSL
LIBS = -lpthread


include $(TOP)/resources/Makefile.in-os

ifeq ($(TARGET_OS),LINUX)
	LIBS += -ldl -lrt
endif
ifdef WITH_ZLIB
	LIBS += -lz
endif

all: $(PROG)

$(PROG): $(CIVETWEB_LIB) $(SRC)
	$(CC) -o websrv $(CFLAGS) $(LDFLAGS) $(SRC) $(CIVETWEB_LIB) $(LIBS) 

$(CIVETWEB_LIB):
	$(MAKE) -C $(TOP) WITH_IPV6=0 WITH_WEBSOCKET=0 NO_SSL=1 clean lib
	cp $(TOP)/$(CIVETWEB_LIB) .

clean:
	rm -f $(CIVETWEB_LIB) $(PROG) websrv

help:
	@echo "make help                show this message"
	@echo "make clean               remove result"
	@echo "make all                 build all"
	@echo "make                     build all"
	@echo ""
	@echo " Make Options:"
	@echo "   NO_SSL=1              build without SSL support"
	@echo ""

.PHONY: all clean
