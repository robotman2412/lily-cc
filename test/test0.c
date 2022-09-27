
#define short char *

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	// A pointer test.
	long *pointer = 1234;
	int numberox = 1;
	
	long a = pointer[numberox];
	// long b = a;
	
	// Halt.
	asm("DEC PC");
}
