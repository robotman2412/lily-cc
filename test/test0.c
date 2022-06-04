
// This is to make vscode not complain about types.
#define short char *

// A function that can be written in the somewhat limited scope.
// unsigned int mul(unsigned int a, unsigned int b) {
// 	unsigned int out;
// 	out = 0;
// 	while (a) {
// 		if (a & 1) out += b;
// 		b <<= 1;
// 		a >>= 1;
// 	}
// 	return out;
// }

// int strlen(short ptr) {
// 	if (!ptr) return 0;
	
// 	short offs;
// 	offs = ptr;
	
// 	while (*ptr) ptr += 1;
	
// 	return ptr - offs;
// }

// void test(int param) {
// 	long long local;
// 	local = 3;
// 	local += param;
// }

void pointers(short param) {
	// long quantum;
	// quantum = *param;
	
	// int  thing;
	// thing = &thing;
	
	param = &param;
}

// int tester() {
// 	asm volatile (
// 		"MOV [0xccdd], 0xfeca"
// 	);
// }

// int iasm() {
// 	asm volatile (
// 		"MOV A, 0\nMOV A, X"
// 	);
// }

// void putc(char c) {
// 	asm volatile (
// 		"MOV [0xfefc], %[reg]"
// 		: // Outputs.
// 		: [reg] "r" (c) // Inputs.
// 	);
// }
