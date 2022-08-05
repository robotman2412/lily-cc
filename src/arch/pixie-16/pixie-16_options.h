
#include "main.h"

// Entrypoint label name.
// If not null, the entry vector is set to this label.
extern const char *entrypoint;

// IRQ handler label name.
// If not null, the IRQ vector is set to this label.
// If null and entrypoint is not null, the IRQ vector will be the same as the entry vector.
// When present, entrypoint is required.
extern const char *irqvector;

// NMI handler label name.
// If not null, the NMI vector is set to this label.
// If null and entrypoint is not null, the NMI vector will be the same as the entry vector.
// When present, entrypoint is required.
extern const char *nmivector;
