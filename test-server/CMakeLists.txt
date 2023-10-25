cmake_minimum_required(VERSION 3.10)

project(test-server LANGUAGES CXX)


file(GLOB SRC_LIST "*.cpp" "*.h")
add_executable(test-server ${SRC_LIST})


#find_package(Libevent 2.2.1 REQUIRED)
target_include_directories(test-server PRIVATE "/usr/local/include")
target_link_libraries(test-server "event_core" "event_extra" "event_pthreads")
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

# target_link_libraries(test-server
# 		  event_extra
# 		  event_core
# 		  event_pthreads
# 		  ${LIB_APPS}
# 		  ${LIB_PLATFORM})

target_compile_options(test-server PRIVATE -fpermissive -std=c++11)
# # OpenSSL::SSL
# message(STATUS "======================      ${LIB_APPS}")
# # ws2_32;shell32;advapi32;bcrypt
# message(STATUS "----------------------      ${LIB_PLATFORM}")