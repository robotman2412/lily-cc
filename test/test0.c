
// This is to make vscode not complain about types.
#define short char *

// A function that can be written in the somewhat limited scope.
// func mul(a, b) {
// 	auto out;
// 	out = 0;
// 	while (a) {
// 		if (a & 1 > 0) out = out + b;
// 		b = b << 1;
// 		a = a >> 1;
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

int iasm() {
	asm volatile (
		"MOV A, 0\nMOV A, Y"
	);
}

// void putc(char c) {
// 	asm volatile (
// 		"MOV [0xfefc], %[reg]"
// 		: // Outputs.
// 		: [reg] "r" (c) // Inputs.
// 	);
// }
