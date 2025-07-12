
cmake_minimum_required(VERSION 3.16.0)

add_subdirectory(test-common)
add_subdirectory(front/c)
add_subdirectory(common)
add_subdirectory(back/riscv)

# Tests for various parts of Lily-CC.
add_executable(lily-test
    main.c
)
target_link_libraries(lily-test PRIVATE
    -Wl,--whole-archive
    compiler-common-test
    c-frontend-test
    riscv-backend-test
    test-common
    -Wl,--no-whole-archive
)
