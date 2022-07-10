cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
project(
	vulkanBasics 
	LANGUAGES C CXX
)

add_subdirectory(src)
add_subdirectory(lib)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC "/W4;")

elseif(GNU OR Clang)
	set(GCC_CLANG_COMPILE_OPTIONS "-Wall;-Wextra;")
	set(GCC_CLANG_COMPILE_OPTIONS_DEBUG "-g;-O0")
	set(GCC_CLANG_COMPILE_OPTIONS_RELEASE "-O3")

	target_compile_options(${PROJECT_NAME} PUBLIC ${GCC_CLANG_COMPILE_OPTIONS})
	target_compile_options(${PROJECT_NAME} PUBLIC $<CONFIG:Debug>:${GCC_CLANG_COMPILE_OPTIONS_DEBUG})
	target_compile_options(${PROJECT_NAME} PUBLIC $<CONFIG:Release>:${GCC_CLANG_COMPILE_OPTIONS_RELEASE})

	if (LINUX)
		target_link_libraries(${PROJECT_NAME} PUBLIC pthreads)
	endif()
	
endif()

find_package(Vulkan REQUIRED)

target_include_directories(
	${PROJECT_NAME} 
	PUBLIC
	"include/"
	"lib/glm/"
	"lib/glfw/include/"
	${Vulkan_INCLUDE_DIR} 
)

target_link_libraries(
	${PROJECT_NAME}
	glfw
	${Vulkan_LIBRARY}
)
