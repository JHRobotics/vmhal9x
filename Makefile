################################################################################
# Copyright (c) 2023-2025 Jaroslav Hensl                                       #
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

vmhal9x.dll:

all: vmhal9x.dll
.PHONY: all clean

OBJ := .o
LIBSUFFIX := .a
LIBPREFIX := lib

DEPS= Makefile config.mk vmhal9x.h mesa3d.h mesa3d_api.h surface.h x86.h memory.h 3d_accel.h
RUNPATH=$(if $(filter $(OS),Windows_NT),.\,./)

HOST_SUFFIX=
ifeq ($(filter $(OS),Windows_NT),Windows_NT)
  HOST_SUFFIX=.exe
endif

DLLFLAGS = -o $@ -shared -Wl,--dll,--out-implib,lib$(@:dll=a),--exclude-all-symbols,--exclude-libs=pthread,--disable-dynamicbase,--disable-nxcompat,--subsystem,windows,--image-base,$(BASE_$@)$(TUNE_LD)

EXEFLAGS = -o $@ -static -nostdlib -nodefaultlibs -lgcc -luser32 -lkernel32 -lgdi32 -lole32 -lshell32 -Wl,-subsystem,windows

LIBS = -luser32 -lkernel32 -lgcc -lgdi32 -ladvapi32
CFLAGS = -std=$(CSTD) -Wall -ffreestanding -fno-exceptions -ffast-math -nostdlib -DNOCRT -DNOCRT_FILE -DNOCRT_FLOAT -DNOCRT_MEM -DNOCRT_CALC -Inocrt -Iregex $(TUNE) -DVMHAL9X_BUILD=$(VERSION_BUILD)
LDFLAGS = -static -nostdlib -nodefaultlibs -L.

ifdef RELEASE
  CFLAGS += -O3 -fomit-frame-pointer -DNDEBUG

  ifdef LTO
    CFLAGS += -flto=auto -fno-fat-lto-objects -pipe -Werror=implicit-function-declaration
    LDFLAGS := $(CFLAGS) $(LDFLAGS)
  endif

  ifdef CODENUKED
    CFLAGS += -DNUKEDCODE
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
BASE_vmdisp9x.dll := 0x32500000

d3d.c.o: d3d_caps.h
mesa3d_buffer.c.o: mesa3d_zconv.h mesa3d_flip.h
mesa3d_nuked.c.o: mesa3d.c mesa3d_buffer.c mesa3d_draw.c mesa3d_chroma.c \
  mesa3d_matrix.c mesa3d_draw6.c mesa3d_dump.c mesa3d_state.c mesa3d_shader.c

NOCRT_OBJS = nocrt/nocrt.c.o nocrt/nocrt_math.c.o nocrt/nocrt_math_calc.c.o \
  nocrt/nocrt_file_win.c.o nocrt/nocrt_mem_win.c.o

VMHAL9X_OBJS = $(NOCRT_OBJS) nocrt/nocrt_dll.c.o vmhal9x.c.o ddraw.c.o 3d_accel.c.o flip32.c.o \
  blt32.c.o rop3.c.o transblt.c.o debug.c.o dump.c.o fill.c.o memory.c.o \
  hotpatch.c.o wine.c.o vmhal9x.res

VMDISP9X_OBJS = $(NOCRT_OBJS) nocrt/nocrt_dll.c.o vmdisp9x.c.o regex/re.c.o vmsetup.c.o vmdisp9x.res

WINETRAY_OBJ = $(NOCRT_OBJS) nocrt/nocrt_exe.c.o tray/tray3d.c.o tray/tray3d.res

ifdef D3DHAL
  ifdef CODENUKED
    VMHAL9X_OBJS += mesa3d_nuked.c.o
  else
	  VMHAL9X_OBJS += d3d.c.o surface.c.o mesa3d.c.o mesa3d_buffer.c.o \
	    mesa3d_draw.c.o mesa3d_chroma.c.o mesa3d_matrix.c.o mesa3d_draw6.c.o \
	    mesa3d_dump.c.o mesa3d_state.c.o mesa3d_shader.c.o
	endif
	CFLAGS += -DD3DHAL
endif

fixlink$(HOST_SUFFIX):
	$(HOST_CC) -std=$(CSTD) fixlink/fixlink.c -o fixlink$(HOST_SUFFIX)

vmdisp9x.dll: $(VMDISP9X_OBJS)
	$(CC) $(LDFLAGS) $(VMDISP9X_OBJS) vmdisp9x.def $(LIBS) $(DLLFLAGS)

tray3d.exe: $(WINETRAY_OBJ)
	$(CC) $(LDFLAGS) $(WINETRAY_OBJ) $(LIBS) $(EXEFLAGS)

vmhal9x.dll: $(VMHAL9X_OBJS) fixlink$(HOST_SUFFIX) vmdisp9x.dll tray3d.exe
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
	-$(RM) $(VMDISP9X_OBJS)
	-$(RM) vmdisp9x.dll
	-$(RM) libvmdisp9x.a
	-$(RM) libddraw.a
	-$(RM) $(WINETRAY_OBJ)
	-$(RM) tray3d.exe
