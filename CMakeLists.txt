project(entropyman)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE source src/*.cpp)
add_executable(entropyman ${source})
