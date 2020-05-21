Basic repository to have a functional environment to start coding in tailored for my needs.
I am not a cmake expert, so I am sure many things may have been a lot cleaner, but it does work for me so `¯\_(ツ)_/¯`.

>
`git submodule update --init --recursive`
`mkdir build`
`cd build`
`cmake -D CMAKE_C_COMPILER=clang-10 -D CMAKE_CXX_COMPILER=clang++-10 -D CMAKE_CXX_FLAGS="-stdlib=libc++" ..`
`make all -j8`

You can also use `make test` to execute all the tests the library comes with, and `make doxygen` to generate its documentation.

By default cmake does not support any target yet. If you want to generate faster code enable O3 optimizations:
>
`cmake -D CMAKE_C_COMPILER=clang-10 -D CMAKE_CXX_COMPILER=clang++-10 -D CMAKE_CXX_FLAGS="-stdlib=libc++ -O3" ..`

- **opt**: the external components.
- **src**: the actual C++ files to be compiled into the exported library.
- **apps**: C++ subprojects to be compiled and linked based on the exported library.
- **test-suite**: C++ sub-projects used to test the library.
- **benchmark-suite**: C++ sub-projects used to test performance of the library.
- **scripts**: folder with utility scripts.
- **header**: .h files for this framework.
- **docs**: the documentation of the project.
- **private**: temporary storage for uncompleted stuff stored only locally.