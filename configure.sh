#!/bin/bash

# Find all the supported archs.
cd src/arch
archs=$(ls -d */ | sed 's#/##')
cd ../..

# Shitty utilities.
CONTAINS() {
    for i in $1 ; do
		if [ "$i" == "$2" ] ; then
			return 0
		fi
	done
	return 1
}

SHOW_ARCHS() {
	for i in $archs ; do
		desc=$(cat src/arch/$i/description.txt)
		echo "  --arch=$i"
		echo "                $desc"
	done
}

HELP() {
	echo "$0 [options]"
	echo "Options:"
	echo "  -a  --arch"
	echo "                Select the architecture to compile for."
	echo "  -h  --help"
	echo "                Show this list."
	# echo "  -h  --help"
	# echo "                SAMPLE_TEXT"
	echo "Architectures:"
	SHOW_ARCHS
}

# Default action.
if [ $# -eq 0 ] ; then
    HELP ; exit
fi

# Some variables
show_help=0
did_options=0
arch=""

# Parse args.
for i in "$@" ; do
	case $i in
		-*) did_options=1 ;;
	esac
	case $i in
		-a=*|--arch=*)
			arch="${i#*=}"
			shift
			;;
		-a*|--arch*)
			echo "Option '$i' missing '=arch-name'"
			show_help=1
			;;
		-h|--help)
			show_help=1
			shift
			;;
		-*)
			echo "Unknown option '$i'"
			show_help=1
			shift
			;;
	esac
done

# Check whether we need to show help.
if [ $did_options == 0 ] || [ $show_help == 1 ] ; then
	HELP; exit
fi

# Check if architecture is valid.
CONTAINS "$archs" $arch || {
	echo "No such architecture '$arch'."
	echo "Architectures:"
	SHOW_ARCHS ; exit
}

# Edit config.h
config_path="src/arch/config.h"
echo "// THIS FILE IS GENERATED, DO NOT EDIT" > "$config_path"
echo "#ifndef CONFIG_H" >> "$config_path"
echo "#define CONFIG_H" >> "$config_path"
echo "#define ARCH_ID \"$arch\"" >> "$config_path"
echo "#include <$arch-config.h>" >> "$config_path"
echo "#include <config-types.h>" >> "$config_path"
echo "#endif" >> "$config_path"

# Edit types.h
types_path="src/arch/types.h"
echo "#ifndef TYPES_H" > "$types_path"
echo "#define TYPES_H" >> "$types_path"
echo "#include <$arch-types.h>" >> "$types_path"
echo "#endif" >> "$types_path"

# Edit gen.c
gen_path="src/arch/gen.c"
echo "// THIS FILE IS GENERATED, DO NOT EDIT" > "$gen_path"
echo "#include <config.h>" >> "$gen_path"
echo "#include <$arch-gen.c>" >> "$gen_path"

# Update current arch thingy.
echo "$arch" > "current_arch"
