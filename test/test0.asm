
// IRQ handler.
	.db entry
// NMI handler
	.db entry
// Entrypoint
	.db entry

	// The entrypoint of the program.
entry:
	ADD R0, 0
