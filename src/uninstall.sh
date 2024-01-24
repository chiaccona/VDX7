#/usr/bin/bash

INSTALLDIR=~/.local/bin
CONFIGDIR=~/.config
INSTALLDIR_LV2=~/.lv2
APP=vdx7

rm -f ${INSTALLDIR}/${APP} 
rm -rf ${CONFIGDIR}/${APP}
rm -rf ${INSTALLDIR_LV2}/${APP}.lv2
