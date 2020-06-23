set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-1 main.cpp)
target_link_libraries(test-1 ${LIBS} ${LOC_LIBS})
add_test(NAME test-1 COMMAND test-1)
