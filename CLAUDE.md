
# First things first
Make sure to *always* read `README.md` to get a better view of the project goals.

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
