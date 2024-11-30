################################################################################
# Copyright (c) 2023-2024 Jaroslav Hensl                                       #
#                                                                              #
# See LICENCE file for law informations                                        #
# See README.md file for more build instructions                               #
#                                                                              #
################################################################################

#
# Usage:
#   1) copy config.mk-sample to config.mk
#   2) edit config.mk
#   3) run make
#
include config.mk

CSTD=gnu99

NULLOUT=$(if $(filter $(OS),Windows_NT),NUL,/dev/null)

VERSION_BUILD = 0
GIT      ?= git
GIT_IS   := $(shell $(GIT) rev-parse --is-inside-work-tree 2> $(NULLOUT))
ifeq ($(GIT_IS),true)
  VERSION_BUILD := $(shell $(GIT) rev-list --count main)
endif

TARGETS = vmhal9x.dll

all: $(TARGETS)
.PHONY: all clean

OBJ := .o
LIBSUFFIX := .a
LIBPREFIX := lib

DEPS=libddraw.a Makefile config.mk vmhal9x.h mesa3d.h mesa3d_api.h d3d_caps.h
RUNPATH=$(if $(filter $(OS),Windows_NT),.\,./)

HOST_SUFFIX=
ifeq ($(filter $(OS),Windows_NT),Windows_NT)
  HOST_SUFFIX=.exe
endif

DLLFLAGS = -o $@ -shared -Wl,--dll,--out-implib,lib$(@:dll=a),--exclude-all-symbols,--exclude-libs=pthread,--disable-dynamicbase,--disable-nxcompat,--subsystem,windows,--image-base,$(BASE_$@)$(TUNE_LD)

LIBS = -luser32 -lkernel32 -lgcc -lgdi32 -ladvapi32 -lddraw
CFLAGS = -std=$(CSTD) -Wall -ffreestanding -fno-exceptions -ffast-math -nostdlib -DNOCRT -DNOCRT_FILE -DNOCRT_FLOAT -DNOCRT_MEM -Inocrt $(TUNE) -DVMHAL9X_BUILD=$(VERSION_BUILD)
LDFLAGS = -static -nostdlib -nodefaultlibs -L.

ifdef RELEASE
  CFLAGS += -O3 -fomit-frame-pointer -DNDEBUG
  
  ifdef LTO
    CFLAGS += -flto=auto -fno-fat-lto-objects -pipe -Werror=implicit-function-declaration
    LDFLAGS := $(CFLAGS) $(LDFLAGS)
  endif
else
  CFLAGS += -O1 -DDDDEBUG=$(DDDEBUG)
  ifdef DEBUG
    CFLAGS += -DDEBUG -g
  else
    CFLAGS += -DNDEBUG
  endif
endif

%.c.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<
		
%.cpp.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
%.res: %.rc $(DEPS)
	$(WINDRES) -DWINDRES -DVMHAL9X_BUILD=$(VERSION_BUILD) --input $< --output $@ --output-format=coff

BASE_vmhal9x.dll := 0xB00B0000
NOCRT_OBJS = nocrt/nocrt.c.o nocrt/nocrt_math.c.o nocrt/nocrt_file_win.c.o nocrt/nocrt_mem_win.c.o nocrt/nocrt_dll.c.o
VMHAL9X_OBJS = $(NOCRT_OBJS) vmhal9x.c.o ddraw.c.o 3d_accel.c.o flip32.c.o blt32.c.o rop3.c.o transblt.c.o debug.c.o dump.c.o fill.c.o vmhal9x.res

ifdef D3DHAL
	VMHAL9X_OBJS += d3d.c.o mesa3d.c.o mesa3d_buffer.c.o mesa3d_draw.c.o mesa3d_chroma.c.o surface.c.o matrix.c.o mesa3d_draw6.c.o
	CFLAGS += -DD3DHAL
endif

fixlink$(HOST_SUFFIX):
	$(HOST_CC) -std=$(CSTD) fixlink/fixlink.c -o fixlink$(HOST_SUFFIX)

vmhal9x.dll: $(VMHAL9X_OBJS) fixlink$(HOST_SUFFIX)
	$(CC) $(LDFLAGS) $(VMHAL9X_OBJS) vmhal9x.def $(LIBS) $(DLLFLAGS)
	$(RUNPATH)fixlink$(HOST_SUFFIX) -shared $@

# generate win9x compatible ddraw import library
libddraw.a: ddraw.def
	$(DLLTOOL) -C -k -d $< -l $@

clean:
	-$(RM) fixlink$(HOST_SUFFIX)
	-$(RM) $(VMHAL9X_OBJS)
	-$(RM) vmhal9x.dll
	-$(RM) libvmhal9x.a
	-$(RM) libddraw.a

