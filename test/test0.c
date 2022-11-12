
#define short char *

// void strtolower(int size, char *src, char *dst) {
// 	for (int i = 0; i < size; ++i) {
		
// 	}
// }

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	// Logic operator test.
	int a, b;
	
	int res = a && b;
	
	// Halt.
	asm("DEC PC");
}
