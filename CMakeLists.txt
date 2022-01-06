project(entropyman)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE source *.cpp)
add_executable(entropyman ${source})
