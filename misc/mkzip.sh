#!/usr/bin/bash

FILE=vdx7_1.0_linux_x86_64.tgz

BASE=${FILE%%.tgz}
tar xzf ../${FILE} && zip ${BASE}.zip $(tar tf ../${FILE} )
rm -rf ${BASE}

