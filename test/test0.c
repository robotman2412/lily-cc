
#define short char *

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	int array[5];
	
	for (int i = 0; i < 5; ++i) {
		int tmp = array[i];
	}
	
	// Halt.
	asm("DEC PC");
}
