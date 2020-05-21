file(GLOB DIRS ./*/CMakeLists.txt)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(${BUILD_CPR_TESTS} OFF)

foreach(T ${DIRS})
	get_filename_component(TS ${T} DIRECTORY)
	add_subdirectory(${TS})
endforeach(T)

set(LIBS ${LIBS} "nlohmann_json::nlohmann_json" CACHE INTERNAL "LIBS")
set(LIBS ${LIBS} "cpr" CACHE INTERNAL "LIBS")
