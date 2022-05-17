
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry

rom:
	//    0 ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' 31
	//  0 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	//  1 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	//  2 - x x x - - - - - - - - - - - - - - - - - - - - - - x - x x x -
	//  3 - x - - x - - - - - - - - - - - - - - - - - - - - - x - x - - -
	//  4 - x - - - x - - - - - - - - - - - - - - - - - - - - x - x x x -
	//  5 - x - - - x - - x - - - - - - - - - x - - - - - - - x - x - x -
	//  6 - x - - x - - - - - - - - - - - - - - - - - - - - - x - x x x -
	//  7 - x - x - - - x x - - x - - - x - x x - - x x x x - - - - - - -
	//  8 - x - - - - - - x - - x - - - x - - x - - x - - - - - - - - - -
	//  9 - x - - - - - - x - - - x - x - - - x - - x - - - - - - - - - -
	// 10 - x - - - - - - x - - - - x - - - - x - - x x x - - - - - - - -
	// 11 - x - - - - - - x - - - x - x - - - x - - x - - - - - - - - - -
	// 12 - x - - - - - - x - - x - - - x - - x - - x - - - - - - - - - -
	// 13 - x - - - - - x x x - x - - - x - x x x - x x x x - - - - - - -
	// 14 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// 15 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	.db 0x0000
	.db 0x3ffc
	.db 0x0004
	.db 0x0084
	.db 0x0048
	.db 0x0030
	.db 0x0000
	.db 0x2080
	.db 0x3fa0
	.db 0x2000
	.db 0x0000
	.db 0x3180
	.db 0x0a00
	.db 0x0400
	.db 0x0a00
	.db 0x3180
	.db 0x0000
	.db 0x2080
	.db 0x3fa0
	.db 0x2000
	.db 0x0000
	.db 0x3f80
	.db 0x2480
	.db 0x2480
	.db 0x2080
	.db 0x0000
	.db 0x007c
	.db 0x0000
	.db 0x007c
	.db 0x0054
	.db 0x0074
	.db 0x0000

entry:
	MOV ST, 0xffff
	
	MOV.JSR PC, func
	; MOV R0, 0
	; MOV [R0+0xffc0], 0x8002
	
	DEC PC

func:
	MOV R0, 0xffe0
.loop:
	MOV R1, [R0+rom+0x0020]
	MOV [R0+0xffe0], R1
	INC R0
	MOV.CC PC, .loop
.exit:
	MOV PC, [ST]

; 	XOR R0, R0
; .loop:
; 	MOV R1, [R0+rom]
; 	MOV [R0+0xffc0], R1
; 	INC R0
; 	CMP R0, 32
; 	MOV.ULT PC, .loop
; .exit:
; 	MOV PC, [ST]
