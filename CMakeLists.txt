cmake_minimum_required(VERSION 3.1)
project(flutter)
find_package(OpenCV REQUIRED)

include(CheckIncludeFiles)
check_include_files(unistd.h HAS_UNISTD_H)
check_include_files(fcntl.h HAS_FCNTL_H)
if(EXISTS "/dev/null" AND ${HAS_UNISTD_H} AND ${HAS_FCNTL_H})
	set(CAN_REDIRECT_TO_DEV_NULL TRUE)
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")

add_executable(flutter flutter.cpp options.cpp)
target_link_libraries(flutter ${OpenCV_LIBS})
target_compile_features(flutter PRIVATE cxx_return_type_deduction)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
