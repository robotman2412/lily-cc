
// This is to make vscode not complain about types.
#define auto int
#define func int

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

// func strlen(ptr) {
// 	auto offs;
// 	offs = ptr;
// 	while (*ptr) ptr = ptr + 1;
// 	return ptr - offs;
// }

// func ptrs(a) {
// 	auto c;
// 	c = &a;
// 	return *c;
// }

func iasm(a) {
	asm (
		"MOV X(0xfefc), A"
	);
}
