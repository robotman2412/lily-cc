
long fibonacci(long depth) {
	if (depth == 0) {
		return 0;
	} else if (depth == 1) {
		return 1;
	} else {
		return fibonacci(depth - 2)
			+ fibonacci(depth - 1);
	}
}

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	long result = fibonacci(6);
	
	asm("DEC PC");
}
