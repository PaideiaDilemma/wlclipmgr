cmake_minimum_required(VERSION 3.1) # setting this is required
project(wlclipmgr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Werror)

add_subdirectory(xdgmime)

add_executable(${PROJECT_NAME}
    main.cpp
    clipboard.cpp
    procblock.cpp
)

target_link_libraries(${PROJECT_NAME} procps)

