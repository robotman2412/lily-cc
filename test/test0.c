
// #define auto int
// #define func int

// func strlen(str) {
// 	auto start;
// 	start = str;
// 	while (*str) {
// 		str = str + 1;
// 	}
// 	return str - start - 1;
// }

func oof(a) {
	return a + 3;
}

// func main() {
// 	return strlen("Excess");
// }

/*
func print(str) {
	while (*str) {
		str = str + 1;
		
		asm ("mov 0xfefc, %0"
			:				// Output.
			: "r" (*str)	// Input.
			: //"memory"	// Clobbered
		);
		// Memory is not clobbered because it is memory-mapped I/O
		// This will tell the compiler to:
		// 1. Evaluate (*str)
		// 2. Move the result to a register
		// 3. Insert the given assembly text
	}
}
*/
