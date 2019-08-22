#!/bin/bash -e
export PS4="$ "
set -x

if [ "$TRAVIS_OS_NAME" == "windows" ]; then
	wget --progress=dot:mega https://bootstrap.pypa.io/get-pip.py
	wget --progress=dot:mega https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-win.zip
	choco install python --version 3.7.4
	if [ "$CC_" == "clang" ]; then
		choco upgrade llvm
	fi
	python get-pip.py
	pip3 install meson
	mkdir /c/tools/ninja-build
	7z x -oC:\\tools\\ninja-build ninja-win.zip
	rm get-pip.py ninja-win.zip
elif [ "$ENGINE" == "meson" ]; then
	wget https://bootstrap.pypa.io/get-pip.py
	wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
	python3.6 get-pip.py --user
	pip3 install --user meson
	unzip ninja-linux.zip -d ~/.local/bin
	rm get-pip.py ninja-linux.zip
fi
