#!/bin/bash

show_help() {
    echo "Usage: $1 [options]"
    echo "Options:"
    echo "  --help"
    echo "                Show this help."
    echo "  --arch=<arch>  -a=<arch>"
    echo "                Set the current architecture."
    echo "  --list  -l"
    echo "                List the available architectures."
    echo
}

list_archs() {
    echo "Architectures:"
    for i in $archs; do
	if [ "$i" != "template" ]; then
            echo "$i"
            cat src/arch/$i/description.txt | sed 's/^/                /'
        fi
    done
    echo
}

cd src/arch
archs=$(echo *)
cd ../..

ver=$(cat version)

# Parse options.
opt_arch=""
opt_list=0
opt_help=0

if [ "$*" = "" ]; then
    show_help $0
    list_archs
    exit
fi
for i in $*; do
    case "$i" in
        --arch=*|-a=*)
            opt_arch="${i#*=}"
            ;;
        --list|-l)
            opt_list=1
            ;;
        --help|-h)
            opt_help=1
            ;;
        -*)
            echo "Error: unknown option '$i'"
            show_help
            break
            ;;
        *)
            break
            ;;
    esac
done

if [ "$opt_help" = 1 ]; then
    show_help $0
fi
if [ "$opt_list" = 1 ]; then
    list_archs
fi
if [ "$opt_arch" = "" ]; then
    exit
fi

# Double-check selection.
acceptable=0
for i in $archs; do
    if [ "$i" = "$opt_arch" ]; then
        acceptable=1; break
    fi
done
if [ "$opt_arch" = "template" -o "$acceptable" = 0 ]; then
    echo "Error: no such architecture '$opt_arch'"
    exit
fi
echo "Configuring lilly-cc v$ver for $opt_arch."

# Apply to files.
if [ ! -d build ]; then
    mkdir build
fi
warn="\
// This is a generated file, do not edit it!
// lilly-cc v$ver ($(date -u))
"

# config.h
echo -n "$warn"                        >  build/config.h
echo "#include <${opt_arch}_config.h>" >> build/config.h

# gen.c
echo -n "$warn"                        >  build/gen.c
echo "#include <${opt_arch}_gen.c>"    >> build/gen.c
echo "#include <gen_fallbacks.c>"      >> build/gen.c

# version_number.h
echo -n "$warn"                        >  build/version_number.h
echo "v$ver"                           >> build/version_number.h

# current_arch
echo -n "$opt_arch"                    >  build/current_arch
