
#define short char *

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	// Try optimise cond mov?
	int thing = 23;
	int other = 91;
	if (thing) {
		other = 1;
	} else {
		other = 2;
	}
	
	// Halt.
	asm("DEC PC");
}
