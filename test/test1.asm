
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry


entry:
	MOV ST, 0xffff
	
	MOV R0, 420
	MOV R1, 69
	MOV.JSR PC, div
	
	DEC PC

	// Multiplies R0 by R1, result in R0.
mul:
	XOR R2, R2
.loop:
	CMP1 R0
	MOV.ULT PC, .exit
	SHR R0
	MOV.CC  PC, .skipadd
	ADD R2, R1
.skipadd:
	SHL R1
	MOV     PC, .loop
.exit:
	MOV R0, R2
	MOV PC, [ST]

	// Divide and modulo R0 by R1, modulo in R0, division in R1.
div:
	// Make R2 (out) and R3 (max) 0.
	XOR R2, R2
	XOR R3, R3
	
	// Find the MAX SHIFT LEFT.
.findmax:
	INC R3
	SHL R1
	// Check divisor < remainder.
	MOV.CC  PC, .findmax
	SHRC R1
	
	// Division loop.
	MOV PC, .divcheck
.divloop:
	SHL R2
	DEC R3
	// Check remainder >= divisor.
	CMP R0, R1
	MOV.ULT PC, .skipsub
	SUB R0, R1
.skipsub:
	SHR R1
	// Check max > 0.
.divcheck:
	CMP1 R3
	MOV.UGE PC, .divloop
	// Div result in R1, mod result in R0.
	MOV R1, R2
	DEC PC
	MOV PC, [ST]


