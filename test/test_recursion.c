
int fibonacci(int depth) {
	if (depth == 0) {
		return 0;
	} else if (depth == 1) {
		return 1;
	} else {
		int x = 0;
		int y = 0;
		int z = 1;
		for (int i = 1; i < depth; ++i) {
			x = y + z;
			y = z;
			z = x;
		}
		return z;
	}
}

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	int result = fibonacci(14);
	
	asm("DEC PC");
}
