
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT



harness_srcs = [
    'testcase.c',
]

harness_include = [
    '.',
]

harness_target = static_library(
    'harness',
    harness_srcs,
    include_directories: harness_include,
    dependencies: [ util_headers_dep ],
)

harness_dep = declare_dependency(
    link_with: harness_target,
    dependencies: [ util_headers_dep ],
    include_directories: harness_include,
)

test_base_deps = [ util_dep, harness_dep ]



test_components = {
    'common': {
        'dependencies': [ common_dep ],
        'files': [
            'sanity-check',
            'arith128/mul128',
            'compiler/srcfile_ram',
            'ir/tokenize',
            'ir/deserialize',
            'ir/append',
            'ir/to_ssa',
            'set/basic',
            'set/multiset',
            'set/large_set',
            'vec/basic',
            'vec/push',
            'vec/bsearch',
            'vec/remove_positions',
            'vec/remove_to_empty',
            'vec/reserve',
            'vec/insert',
            'vec/cstr',
        ],
    },
    'front/c': {
        'dependencies': [ front_c_dep ],
        'files': [
            'tokenizer/basic',
            'tokenizer/litsuffix',
            'tokenizer/errors',
            'compiler/exprs',
            'compiler/types',
            'parser/expr_prefix',
            'parser/expr_basic',
            'parser/expr_call',
            'parser/expr_deref',
            'parser/expr_cast',
            'parser/type_funcptr',
            'parser/type_struct',
            'parser/type_enum',
            'parser/stmt_decl',
            'parser/stmt_ctrl',
            'parser/function',
            'parser/compliteral',
            'preproc/counter',
            'preproc/object_macros',
            'preproc/object_paste_and_hash',
            'preproc/line_continuation',
            'preproc/func_basic',
            'preproc/va_args',
            'preproc/va_opt',
            'preproc/func_paste',
            'preproc/stringize',
            'preproc/self_reference',
            'preproc/nested_calls',
            'preproc/if_arith',
            'preproc/defined_op',
            'preproc/if_branches',
        ],
    },
    'back/riscv': {
        'dependencies': [ back_riscv_dep ],
        'files': [
            'codegen/isel',
        ],
    },
}

foreach component, dict : test_components
    foreach file : dict['files']
        path = component + '/' + file + '.c'
        id = (component.replace('/', '-') + '-' + file.replace('/', '-'))
        exec = executable(
            id, path,
            install: false,
            dependencies: dict['dependencies'] + test_base_deps
        )
        test(id, exec)
    endforeach
endforeach
