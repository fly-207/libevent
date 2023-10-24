cmake_minimum_required(VERSION 3.10)

project(test-server LANGUAGES CXX)



add_executable(test-server "main.cpp")


find_package(Libevent 2.2.1 REQUIRED)
target_include_directories(test-server PRIVATE ${LIBEVENT_INCLUDE_DIR})
target_link_libraries(test-server ${LIBEVENT_LIBRARY})


# target_link_libraries(test-server
# 		  event_extra
# 		  event_core
# 		  event_pthreads
# 		  ${LIB_APPS}
# 		  ${LIB_PLATFORM})

# target_compile_options(test-server PRIVATE -fpermissive)
# # OpenSSL::SSL
# message(STATUS "======================      ${LIB_APPS}")
# # ws2_32;shell32;advapi32;bcrypt
# message(STATUS "----------------------      ${LIB_PLATFORM}")