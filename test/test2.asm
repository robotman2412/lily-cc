
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry

entry:
	MOV R0, 0x8000 // Comment on EOL will no longer break it.
	MOV.CX R1, R0
	MOV R2, 0x7fff
	MOV.CX R3, R2
	DEC PC
