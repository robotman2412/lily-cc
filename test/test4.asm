
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry

rom:
	.db "Hello, World!", 0

entry:
	MOV ST, 0xff00
	
	MOV R0, rom
	MOV.JSR PC, print
	
	DEC PC

print:
	MOV PC, .check
.loop:
	MOV R1, [R0]
	MOV [0xfff6], R1
	INC R0
.check:
	CMP1 [R0]
	MOV.UGE PC, .loop
	MOV PC, [ST]
