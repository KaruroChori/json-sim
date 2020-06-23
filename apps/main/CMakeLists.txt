set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cpp)
target_link_libraries(main ${LIBS} ${LOC_LIBS})
