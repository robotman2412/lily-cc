
#define short char *

// int strlen(short pointer) {
// 	short initial = pointer;
// 	while (*pointer) ++pointer;
// 	return pointer - initial;
// }

void entry() {
	// Initialise stack.
	asm("MOV ST, 0xffff");
	asm("SUB ST, [0xffff]");
	
	short str = "World, Hello?";
	// int quantum = strlen(str);
	
	// while (quantum) {
	// 	*0xfff6 = *str;
	// 	++str;
	// 	--quantum;
	// }
	
	*0xfff6 = str[0];
	
}
