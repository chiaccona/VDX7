#  
#  VDX7 - Virtual DX7 synthesizer emulation
#  Copyright (C) 2023  chiaccona@gmail.com 
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#  


# Requires (e.g. debian) packages:
#   build_essential (supporting g++ -std=c++-17)
#   libx11-dev
#   libxpm-dev
#   libsamplerate0-dev
#   libjack-jackd2-dev (for app)
# Optional:
#   lv2-dev (1.16+, for experimental LV2)
#   libgtkmm-3.0-dev and all it's deps (for experimental GTK3 GUI - NOT recommended)

##################################
##### Configuration options: #####
##################################
USE_NATIVE=0
USE_LV2=1
USE_ARM=0
USE_GTKMM=0
##################################

APP=vdx7
VERSION=1.0
BUILD=build$(shell date +%m%d%y-%H%M%S)

####### INSTALLATION #############
INSTALLDIR= ~/.local/bin
CONFIGDIR= ~/.config/vdx7
INSTALLDIR_LV2= ~/.lv2/$(APP).lv2

##################################
##################################
##################################


##################################
# create archive with "vdx7-1.0/" top directory
#NO_TARBOMB= 
NO_TARBOMB= --xform s:^:$(APP)-$(VERSION)/: 
##################################


LV2_EXEC=$(APP).so
APP_EXEC=$(APP)
MANIFEST= $(APP).ttl manifest.ttl

####### COMPILER ###########
SSE= -msse -msse2 -mfpmath=sse
#SSE+= -ftree-vectorize -funsafe-math-optimizations

NEON= -mcpu=cortex-a53
NEON+= -mfpu=neon-vfpv4
#NEON+= -ftree-vectorize -funsafe-math-optimizations

CXX=g++
CC=gcc

##################################
############ Sources #############
##################################

ifeq ($(USE_GTKMM),1)
GUI_CC=GTK/Gui-gtkmm.cc GTK/resources.c
else
GUI_CC=Gui.cc Widgets.cc
endif

COMMON_SRCS = Synth.cc HD6303R.cc HD6303R_inst.cc dx7.cc firmware.bin voices.bin

LV2_SRCS = $(COMMON_SRCS) lv2_plugin.cc lv2_gui.cc $(GUI_CC) 
APP_SRCS = $(COMMON_SRCS) main.cc $(GUI_CC) JackDriver.cc 


##################################
############ C Flags #############
##################################

CXXFLAGS=
CFLAGS=
CPPFLAGS=

CFLAGS+= -g -O3 -Wall -pipe
CFLAGS+= -fomit-frame-pointer 
CFLAGS+= -ffast-math -fno-finite-math-only

ifeq ($(USE_NATIVE),1)
	CFLAGS+= -march=native
else
	CFLAGS+= -mtune=generic
endif

CXXFLAGS+= -std=c++17

ifeq ($(USE_ARM),1)
	CFLAGS+=$(NEON)
else
	CFLAGS+=$(SSE)
endif

# Compile a version number into the code
CPPFLAGS+=-DVERSION=$(VERSION)$(BUILD)

# Force G++ to inline the EGS and OPS clock functions
CPPFLAGS+=-DINLINE_EGS

CFLAGS+= -fPIC -DPIC

LV2_CFLAGS= -fvisibility=hidden -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed \
	-shared `pkg-config --cflags lv2`

ifeq ($(USE_GTKMM),1)
CFLAGS+= -IGTK `pkg-config --cflags gtkmm-3.0` -DGTKMM=1
else
CFLAGS+= -DGTKMM=0
endif

CXXFLAGS += $(CFLAGS)

##################################
############ LD Flags ############
##################################

LDFLAGS= -lm
LDFLAGS+= -lsamplerate
LDFLAGS+= -lpthread

# Allow app to execute from Nemo, grrr...  :-(
#LDFLAGS+= -no-pie

ifeq ($(USE_GTKMM),1)
LDFLAGS+= `pkg-config --libs gtkmm-3.0`
else
LDFLAGS+= -lX11 -lXpm
endif


LV2_LDFLAGS= $(LDFLAGS) `pkg-config --libs lv2`
APP_LDFLAGS= $(LDFLAGS) -ljack 

##################################
############# Build ##############
##################################
ifeq ($(USE_LV2),1)
all: app lv2
else
all: app
endif
app: $(APP_EXEC)
lv2: $(LV2_EXEC)

################################################################
######################### Machinery ############################
################################################################

BUILD_DIR ?= ./.build
RELEASE=$(APP)_$(VERSION)_linux_x86_64
RELEASE_DIR=$(BUILD_DIR)/$(RELEASE)

SRC_DIRS ?= .

APP_OBJS := $(APP_SRCS:%=$(BUILD_DIR)/%.o)
APP_DEPS := $(APP_OBJS:.o=.d)

LV2_OBJS := $(LV2_SRCS:%=$(BUILD_DIR)/%.o)
LV2_DEPS := $(LV2_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FILES := $(shell find $(SRC_DIRS) -name \*.h)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

#CPPFLAGS += $(INC_FLAGS)
CPPFLAGS += -MMD -MP

$(APP_EXEC): $(APP_OBJS)
	$(CXX) $(CXXFLAGS) $(APP_OBJS) -o $@ $(APP_LDFLAGS)

$(LV2_EXEC): $(LV2_OBJS)
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) $(LV2_OBJS) -o $@ $(LV2_LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	@$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cc.o: %.cc
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# bin source
$(BUILD_DIR)/%.bin.o: %.bin
	@$(MKDIR_P) $(dir $@)
	ld -r -b binary $< -o $@

.PHONY: clean

-include $(LV2_DEPS) $(APP_DEPS)

MKDIR_P ?= mkdir -p

################################################################
############################ Misc ##############################
################################################################

install: install-app install-lv2

install-app:
	@$(MKDIR_P) $(INSTALLDIR)
	cp $(APP) $(INSTALLDIR)
	@$(MKDIR_P) $(CONFIGDIR)
	if ! test -f $(CONFIGDIR)/vdx7.ram ; then cp example.ram $(CONFIGDIR)/vdx7.ram ; fi

install-lv2: $(MANIFEST) $(LV2_EXEC)
	@$(MKDIR_P) $(INSTALLDIR_LV2)
ifeq ($(USE_GTKMM),1)
	@sed 's/@TYPE@/Gtk3UI/' manifest.ttl.in > manifest.ttl
else
	@sed 's/@TYPE@/X11UI/' manifest.ttl.in > manifest.ttl
endif
	cp $(MANIFEST) $(LV2_EXEC) $(INSTALLDIR_LV2)
	if ! test -f $(INSTALLDIR_LV2)/vdx7.ram ; then cp example.ram $(INSTALLDIR_LV2)/vdx7.ram ; fi

clean:
	$(RM) -r $(BUILD_DIR) $(APP_EXEC) $(LV2_EXEC)

release: $(MANIFEST) $(LV2_EXEC)
	$(RM) -r $(RELEASE_DIR)
	@$(MKDIR_P) $(RELEASE_DIR)
	@$(MKDIR_P) $(RELEASE_DIR)
	cp $(APP) $(RELEASE_DIR)
	strip $(RELEASE_DIR)/$(APP)
	cp ../README.md ../README.FAQ.md ../LICENSE ../COPYRIGHT  \
		../misc/install.sh ../misc/uninstall.sh ../misc/VDX7.png ../misc/VDX7.desktop $(RELEASE_DIR)
	cp example.ram $(RELEASE_DIR)/vdx7.ram
ifeq ($(USE_LV2),1)
	@$(MKDIR_P) $(RELEASE_DIR)/$(APP).lv2
	cp $(MANIFEST) $(LV2_EXEC) $(RELEASE_DIR)/$(APP).lv2
	strip $(RELEASE_DIR)/$(APP).lv2/$(LV2_EXEC)
	cp example.ram $(RELEASE_DIR)/$(APP).lv2/vdx7.ram
endif
	( cd $(BUILD_DIR) ; tar -czvf ../../$(RELEASE).tgz $(RELEASE) )

tar:
	tar $(NO_TARBOMB) -czvf $(APP)_$(VERSION)_$(shell date +%m%d%y-%H%M%S).tgz \
		*.h *.cc Makefile \
		firmware.bin voices.bin panel.xpm *.ttl* \
		GTK/fader.png GTK/style.css GTK/image.gresource.xml GTK/*.cc GTK/*.h \
		example.ram ../ChangeLog ../README* ../LICENSE ../COPYRIGHT 

GTK/resources.c: GTK/image.gresource.xml GTK/fader.png
	( cd GTK; glib-compile-resources image.gresource.xml --generate-source --target=resources.c )

