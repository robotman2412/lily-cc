
; func mul(a, b) {
; 	auto out;
; 	out = 0;
; 	while (a) {
; 		if (a & 1) out = out + b;
; 		b = b << 1;
; 		a = a >> 1;
; 	}
; 	return out;
; }

// Compiler output: 42 + 104*n
// Worst case: 1706 (1.7ms)
	.section ".text"
// function entry
	.section ".bss"
mul.LA0000:
	.zero 0x2
mul.LA0001:
	.zero 0x2
mul.LV0000:
	.zero 0x2
	.section ".text"
mul:
// function code
	MOV  A, 0x00			// 2
	MOV  [.LV0000+0], A		// 4
	MOV  A, 0x00			// 2
	MOV  [.LV0000+1], A		// 4
	JMP mul.L1				// 4
mul.L0:
// Add temp label mul.LT0000
	.section ".bss"
mul.LT0000:
	.zero 0x2
	.section ".text"
	MOV A, [mul.LA0000+0]	// 4
	AND A, 0x01				// 3
	MOV [mul.LT0000+0], A	// 4
	MOV A, [mul.LA0000+1]	// 4
	AND A, 0x00				// 3
	MOV [mul.LT0000+1], A	// 4
	MOV A, [mul.LT0000+0]	// 4
	CMP A, 0x00				// 3
	MOV A, [mul.LT0000+1]	// 4
	CMPC A, 0x00			// 3
	BLE mul.L2				// 4
	MOV A, [mul.LV0000+0]	// 4
	ADD A, [mul.LA0001+0]	// 5
	MOV [mul.LV0000+0], A	// 4
	MOV A, [mul.LV0000+1]	// 5
	ADDC A, [mul.LA0001+1]	// 5
	MOV [mul.LV0000+1], A	// 4
mul.L2:
	SHL [mul.LA0001+0]		// 6
	SHLC [mul.LA0001+1]		// 6
	SHR [mul.LA0000+1]		// 6
	SHRC [mul.LA0000+0]		// 6
mul.L1:
	MOV A, [mul.LA0000+0]	// 4
	OR A, [mul.LA0000+1]	// 5
	BNE mul.L0				// 4
	MOV X, [mul.LV0000+0]	// 4
	MOV Y, [mul.LV0000+1]	// 4
	RET						// 5
// return was explicit

// Optimal assembly: 40 + 67*n
// Worst case: 1112 (1.1ms)
// Performs +35.6% better
mul:
	.section ".bss"
.a:	.zero 2
.b: .zero 2
.c: .zero 2
	.section ".text"
	// auto c = 0;
	MOV  A, 0x00			// 2
	MOV  [.c + 0], A		// 4
	MOV  [.c + 1], A		// 4
	// while ... {code}
	JMP  .loop_chk			// 4
.loop:
	// if (a & 1) c += b; a >>= 1;
	SHR  [.a + 1]			// 6
	SHRC [.a + 0]			// 6
	BCC  .no_add			// 4
	// c += b;
	MOV  A, [.b + 0]		// 4
	ADD  A, [.c + 0]		// 5
	MOV  [.c + 0], A		// 4
	MOV  A, [.b + 1]		// 4
	ADDC A, [.c + 1]		// 5
	MOV  [.c + 1], A		// 4
.no_add:
	SHL  [.b + 0]			// 6
	SHLC [.b + 1]			// 6
	// while (expr) ...
.loop_chk:
	MOV  A, [.a + 0]		// 4
	OR   A, [.a + 1]		// 5
	BNE  .loop				// 4
	// return c;
	MOV  X, [.c + 0]		// 4
	MOV  Y, [.c + 1]		// 4
	RET						// 5

