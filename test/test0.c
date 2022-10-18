
#define short char *

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	// A pointer test.
	// int arr[5];
	// arr[3] = 91;
	int *q;
	*q = 5;
	
	// Halt.
	asm("DEC PC");
}
