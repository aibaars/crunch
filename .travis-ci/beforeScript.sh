#!/bin/bash -e
unset PS4
set -x

if [ "$ENGINE" == "make" ]; then
	make lib
	sudo -E make install-so
	make -C crunch crunch
	make -C crunch++ crunch++
	make -C crunchMake
	sudo -E make install
elif [ "$ENGINE" == "meson" ]; then
	CC="$CC_" CXX="$CXX_" meson build --prefix=$HOME/.local
	cd build
	ninja
fi
