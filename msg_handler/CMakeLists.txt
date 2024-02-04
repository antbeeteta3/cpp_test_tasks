cmake_minimum_required (VERSION 3.22)

project (msg_handler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=return-type -Werror=missing-field-initializers")

#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)
find_package(fmt REQUIRED)	# fmtlib
find_package(Protobuf REQUIRED)

set(SRC_FILES
	main.cpp
	udp_transport.cpp
	poller.cpp
	protocol.cpp
	device_info.cpp
	generated/message.pb.cc
)

add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/generated
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	#Threads::Threads
	fmt::fmt
	${Protobuf_LIBRARIES}
)
