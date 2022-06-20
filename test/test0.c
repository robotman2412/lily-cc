
// This is to make vscode not complain about types.
#define short char *

// int strlen(short ptr) {
// 	if (!ptr) return 0;
	
// 	short offs;
// 	offs = ptr;
	
// 	while (*ptr) ptr += 1;
	
// 	return ptr - offs;
// }

// Dummy function to create my raw data.
void dummy() {
	asm(".db 0, 0, entry");
}

void entry() {
	int str = "Hello, World!";
	asm(
		"MOV [0xfff6], %[reg]"
		: // Outputs.
		: [reg] "ri" ('H') // Inputs.
	);
	// Halt.
	asm("DEC PC");
}

// void test(int param) {
// 	long long local;
// 	local = 3;
// 	local += param;
// }

// void pointers(short param) {
// 	long quantum;
// 	quantum = *param;
	
// 	int  thing;
// 	thing = &thing;
	
// 	param = &param;
// }

// void putc(char c) {
// 	asm volatile (
// 		"MOV [0xfefc], %[reg]"
// 		: // Outputs.
// 		: [reg] "r" (c) // Inputs.
// 	);
// }
