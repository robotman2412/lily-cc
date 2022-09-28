
#define short char *

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	// A pointer test.	
	int *arrayOfPointers[];
	int (*pointerOfArrays)[];
	
	// Halt.
	asm("DEC PC");
}
