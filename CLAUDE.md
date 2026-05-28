# Specifications

They are the source of truth; **make good use of these!**

- `~/Sync/datasheets/software specifications/c23.pdf` C23 language specification

The above specifications do not require approval; read them whenever you need to know about C semantics; they are to be treated as more trustworthy than the compiler's source code.

# Structure

The project is structured into multiple folders under `src`:

- `main` - main executables `lilycc`, `lily-cpp` and `lily-explainer`
- `compiler` - compiler as a library
  - `back/riscv` - RISC-V backend
  - `common` - compiler middleware library
    - `back` - generic backend code
    - `ir` - assembly-like intermediate representation and generic optimizer (types largely erased)
    - `front` - generic frontend code
  - `front/c` - C23 frontend
    - `c_grammar` - the C AST and parser
    - `c_semantic` - semantic compiler that outputs a high-level IR (types completely preserved)

# Commands

Please refrain from using other commands, primarily because of the need for manual approval:

- `make test` - run the test code (requires approval)
- `make valgrind-test` - run the test code under valgrind (requires approval)
- `make` - compile everything
- `make build` - compile everything except the tests
