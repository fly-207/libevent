cmake_minimum_required(VERSION 3.10)

project(test-server LANGUAGES CXX)

file(GLOB SRC_LIST "*.cpp" "*.h")

add_executable(test-server ${SRC_LIST})

target_link_libraries(test-server
		  event_extra
		  event_core
		  ${LIB_APPS}
		  ${LIB_PLATFORM})

# OpenSSL::SSL
message(STATUS "======================      ${LIB_APPS}")
# ws2_32;shell32;advapi32;bcrypt
message(STATUS "----------------------      ${LIB_PLATFORM}")