
// IRQ vector.
	.db -1
// NMI vector.
	.db -1
// Entry vector.
	.db entry


entry:
	MOV ST, 0xfffe
	MOV [0xfffe], 0xc000
	MOV PC, [ST]
