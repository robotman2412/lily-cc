# Slow down there kiddo
A compiler for a very simple language

## Developing for lilly-c
1. Pull git git repository: `git clone https://github.com/robotman2412/lilly-c`
2. Configure (see [Configuration](#Configuration))
3. Build for the first time: `./build.sh` <sup>(1)</sup>
4. The output executable is called `comp` <sup>(2)</sup>

Note 1: The initial build is required because several files are generated.

Note 2: Executable name subject to change.

## Configuration
There are two options at this point:

### Configure for an existing architecture:
1. To list help and architectures: `./configure.sh --help`
2. Pick an architecture and configure: `./configure.sh --arch=mos6502`

Note: MOS 6502 is not implemented at the moment.

### Create a new architecture:
1. Think of an appropriate architecture ID, for example: `8086`
1. Create a directory with the desired architecture ID: `src/arch/8086`
2. Create a `decription.txt` there, for example: `Intel 8086 desktop CPU` <sup>(1)</sup>
3. Write `8086-config.h`, `8086-gen.c` and `8086-types.h` accordingly. <sup>(2)</sup>

Note 1: Description files should have exactly one trailing newline.

Note 2: Refer to `src/arch/gr8cpu-r3` for examples of implementing these files.

