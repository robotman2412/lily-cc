# lily-cc
![Icon image](icon.png)

A work-in-progress C compiler for small CPUs.

## Installing lily-cc
1. Pull git repository: `git clone https://github.com/robotman2412/lily-cc && cd lily-cc`
2. Configure (see [Configuration](#Configuration))
3. Build for the first time: `make`
4. Install the compiler: `sudo ./install.sh`

## Developing for lily-cc
1. Pull git repository: `git clone https://github.com/robotman2412/lily-cc && cd lily-cc`
2. Configure (see [Configuration](#Configuration))
3. Build for the first time: `make` <sup>(1)</sup>
4. The output executable is called `comp`
5. Happy editing!

Note 1: The initial build is required for IDE users because several files are generated.

## Configuration
Confirguring is required to select the target architecture.

There are two options at this point:

### Configure for an existing architecture:
 - To list help and architectures: `./configure.sh --help`
 - To pick an architecture and configure: `./configure.sh --arch=pixie-16`

### Create a new architecture:
1. Think of an appropriate architecture ID, for example: `8086`
2. Create a directory with the desired architecture ID: `src/arch/8086`
3. Create a `decription.txt` there, for example: `Intel 8086 desktop CPU` <sup>(2)</sup>
4. Write `8086_config.h`, `8086_gen.c` and `8086_gen.h` accordingly. <sup>(3)</sup>
5. Configure for your newly made architecture: `./configure.sh --arch=8086`

Note 2: Description files should have exactly one trailing newline.

Note 3: Look to arch `pixie-16` for examples.
