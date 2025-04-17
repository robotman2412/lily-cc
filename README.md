# The Lily C compiler
![Icon image](icon.png)

A work-in-progress C compiler for fun. It aims to one day be C23 (and some GNU extensions) compliant.

## Current state of progress
Most of C tokenization, parsing and compilation into Lily's assembly-like intermediate representation (IR) is finished.
The RISC-V instructions that will be supported have been defined in terms of IR semantics and
the next goal is to create codegen capable of turning the IR into such machine instructions given these semantics.

For more detail see [TODO.md](TODO.md).
