cmake_minimum_required (VERSION 3.16)

project (lru)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=return-type -Werror=missing-field-initializers")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


set(SRC_FILES
	main.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES} )

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC TEST_OUT)

target_link_libraries(${CMAKE_PROJECT_NAME}
	Threads::Threads
)