
func mul(a, b) {
	b = b + 3;
	return b;
}

mul:
	PSH Y
	
	MOV Y, A
	MOV A, X
	ADD A, 0x03
	MOV X, A
	MOV A, X
	
	PUL Y
	RET

target:
	PSH Y
	
	ADD X, 0x03
	MOV A, X
	
	PUL Y
	RET
