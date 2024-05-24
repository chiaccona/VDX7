#!/usr/bin/bash

# By default, installs to user's local directories
# Change these as appropriate
INSTALLDIR=~/.local
CONFIGDIR=~/.config
INSTALLDIR_LV2=~/.lv2
DESKTOP=~/Desktop
APP=vdx7


# Copy files

# App
mkdir -p ${INSTALLDIR}/bin
cp ${APP} ${INSTALLDIR}/bin

# Desktop
mkdir -p ${INSTALLDIR}/share/icons/hicolor/128x128/apps
cp ${APP}.png ${INSTALLDIR}/share/icons/hicolor/128x128/apps
if test -d ${DESKTOP} ; then
	cp ${APP}.desktop ${DESKTOP}
fi

# config file
mkdir -p ${CONFIGDIR}/${APP}
cp ${APP}.ram ${CONFIGDIR}/${APP}

# LV2
if test -d ${APP}.lv2 ; then
	mkdir -p ${INSTALLDIR_LV2}
	cp -r ${APP}.lv2 ${INSTALLDIR_LV2}
fi
