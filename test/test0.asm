
; func mul(a, b) {
; 	var out = 0;
; 	while (a) {
; 		out = out + b;
; 		a --;
; 	}
; 	return out;
; }

mul:
	PSH Y
	MOV Y, 0x00
L0:
	CMP A, 0x00
	BEQ L1		; PIE
	PSH A
	MOV A, Y
	ADD A, X
	MOV Y, A
	PUL A
	DEC A
	JMP L0		; PIE
L1:
	MOV A, Y
	PUL Y
	RET

optimal:
	PSH Y
	MOV Y, A
	SUB A, Y
	CMP A, Y
	BEQ L1		; PIE
L0:
	ADD A, X
	DEC Y
	BGE L0		; PIE
L1:
	PUL Y
	RET

; void tfw(int a) {
; 	int out = 0;
; 	for (; a; a--) {
; 		out ++;
; 	}
; 	return out;
; }

tfw:
	PSH Y
	MOV X, 0x00
.L0: ; 0x0003
	CMP A, 0x00
	BEQ .L1 ; PIE
	INC X
	DEC A
	JMP .L0 ; PIE
.L1: ; 0x000d
	MOV A, X
	PUL Y
	RET


tfw:
	PSH Y
	MOV X, 0x00
	; {
L0:
		; {
			CMP A, 0x00
			BEQ L1
			; {
				INC X
			; }
			DEC A
		; }
		JMP L0
L1:
	; }
	MOV A, X
	PUL Y
	RET

