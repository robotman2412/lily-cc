
cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(lily-cc)
add_compile_options(-std=gnu17 -Og -ggdb)


add_subdirectory(util)

add_subdirectory(common)
target_link_libraries(compiler-common PUBLIC util)

add_subdirectory(front/c)
target_link_libraries(c-frontend PUBLIC compiler-common)

# add_subdirectory(back/faerie)
# target_link_libraries(faerie-backend PUBLIC compiler-common)


add_executable(lily-cc
    main/main.c
)
target_include_directories(lily-cc PUBLIC main)
target_link_libraries(lily-cc PUBLIC c-frontend)


add_executable(lily-cc-test
    test/main.c
    test/common/frontend.c
    test/front/c/c_tokenizer.c
)
target_include_directories(lily-cc-test PUBLIC test)
target_link_libraries(lily-cc-test PUBLIC c-frontend)
