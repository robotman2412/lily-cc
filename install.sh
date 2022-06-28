#!/bin/bash

./configure.sh --check || exit 1

ARCH=$(cat build/current_arch)
names=$(cat src/arch/$ARCH/bin_names.txt)

if [[ "$(whoami)" != "root" ]]; then
    echo "Error: Root required to copy to /usr/bin"
    exit 2
fi

for name in $names; do
    path="/usr/bin/$name"
    echo "Installing to $path..."
    cp ./comp "$path"
done
echo "Done!"
