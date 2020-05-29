#!/usr/bin/env bash

if [ "$1" = "clean" ]; then
    echo Cleaning...
    rm -rf build
    exit 0
fi

if [ "$1" = "install" ]; then
    echo Installing...
    sudo dpkg --install build/*.deb
    exit 0
fi

if [ "$1" = "update" ]; then
	echo Updating EXE...
	sudo cp build/openjvs /usr/bin/openjvs
	exit 0
fi

if "$1" = "develop" ]; then
	echo Setting up symlinks..
	sudo ln -s $(pwd)/docs/openjvs/ /etc/
	exit 0
fi

mkdir -p build
pushd build
cmake ..
cmake --build .
cpack
popd

