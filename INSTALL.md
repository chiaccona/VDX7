# Install

Building is straightforward. The only dependencies for building the app are the
dev packages for X11 (including xpm), jack2, and libsamplerate. GCC's std-c++17
is required, along with std::filesystem (g++-8 or higher).  The LV2 plugin
requires the dev package for LV2 1.16 or higher.  The top of the Makefile has
several switches that can be chosen, including X86 or ARM architectures.

Deps and config options:
```
# Requires (e.g. debian) packages:
#   build_essential (supporting g++ -std=c++-17)
#   libx11-dev
#   libxpm-dev
#   libsamplerate0-dev
#   libjack-jackd2-dev (for app)
# Optional:
#   lv2-dev (1.16+, for experimental LV2)
#   libgtkmm-3.0-dev (for experimental GTK3 GUI - NOT recommended)

##################################
##### Configuration options: #####
##################################
USE_NATIVE=1
USE_LV2=1
USE_ARM=0
USE_GTKMM=0
##################################

####### INSTALLATION #############
INSTALLDIR= ~/.local/bin
CONFIGDIR= ~/.config/vdx7
INSTALLDIR_LV2= ~/.lv2/$(APP).lv2
##################################
```

Build:
```
make && make install
```

Set the Configuration Options flags to 0 or 1 as appropriate.

By default the install directory is the user's `~/.local/bin`. The Makefile can
be changed to direct to a different location.  LV2 is also install by default
in `~/.lv2` directory. 

A default RAM image file is installed in `~/.config/vdx7`. This file should
always be local to the user (even if the app and LV2 are installed in a system
location)  - the app looks for it first by checking in `$XDG_CONFIG_HOME/vdx7`, then
`~/.config/vdx7`.

By default the "-march=native" gcc flag is invoked, which will optimize for the architecture of the machine it is built on, and likely not run (or run well) on a different machine. Comment this out (or change suitably) if building for e.g. a generic x86 architectures.


