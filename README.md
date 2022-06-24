# lily-cc
![Icon image](icon.png)

A work-in-progress C compiler for small CPUs.

## Developing for lily-cc
1. Pull git repository: `git clone https://github.com/robotman2412/lily-cc`
2. Configure (see [Configuration](#Configuration))
3. Build for the first time: `make` <sup>(1)</sup>
4. The output executable is called `comp`
5. Happy editing!

Note 1: The initial build is required for IDE users because several files are generated.

## Configuration
There are two options at this point:

### Configure for an existing architecture:
1. To list help and architectures: `./configure.sh --help`
2. Pick an architecture and configure: `./configure.sh --arch=gr8cpu-r3`

### Create a new architecture:
1. Think of an appropriate architecture ID, for example: `8086`
2. Create a directory with the desired architecture ID: `src/arch/8086`
3. Create a `decription.txt` there, for example: `Intel 8086 desktop CPU` <sup>(1)</sup>
4. Write `8086_config.h`, `8086_gen.c` and `8086_gen.h` accordingly. <sup>(2)</sup>
5. Configure for your newly made architecture: `./configure.sh --arch=8086`

Note 1: Description files should have exactly one trailing newline.

Note 2: Look to arch `pixie-16` for examples.
