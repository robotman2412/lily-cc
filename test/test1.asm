
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry


entry:
	MOV ST, 0xffff
	
	MOV R0, 21
	MOV R1, 11
	MOV.JSR PC, mul
	
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
	XOR R2, R2
	// Mask R3 to be the highest bit of R0.
	MOV R3, R0
	DEC R3
	AND R3, R0
	// Shift left R1 until it's as big as R3.
.maximise:
	CMP R1, R3
	MOV.UGE PC, .next
	SHL R1
	INC R2
	MOV PC, .maximise
	// Prepare for division.
.next:
	XOR R3, R3
	// Division loop.
.divloop:
	CMP R0, R1
	MOV.ULT PC, .skipsub
	SUB R0, R1
	INC R3
.skipsub:
	
