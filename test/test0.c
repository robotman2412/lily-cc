
// Type support is low, so pointers are just ints.

// A very simple function that outputs msg to the debug TTY.
void print(short msg) {
	// While there is data to print...
	while (*msg) {
		// Move from message pointer (msg) to debug output (MMIO 0xfff6).
		*0xfff6 = *msg;
		// Next character.
		msg += 1;
	}
}

// The entrypoint function.
// Entrypoint may not have parameters nor return type, and must never return.
// 
void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	print("Hello, World!\n");
	print("This is a second message!\n");
	while (1) {
		print("This message repeats.\n");
	}
	
	// Return is not allowed (nothing to return to),
	// but this point will not be reached.
}
