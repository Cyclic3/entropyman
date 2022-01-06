project(entropyman)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE source *.cpp)
add_executable(entropyman ${source})

find_library(TCMALLOC_LIB NAMES libtcmalloc.so.4)

target_link_libraries(entropyman PUBLIC ${TCMALLOC_LIB})
