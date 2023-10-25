cmake_minimum_required(VERSION 3.10)

project(test-server LANGUAGES CXX)


file(GLOB SRC_LIST "*.cpp" "*.h" "*.cc")
message(STATUS "======================    SRC_LIST=${SRC_LIST}")
add_executable(test-server ${SRC_LIST})

# 不要使用 find_package, 有依赖库, 会形成一连串的问题
#find_package(Libevent 2.2.1 REQUIRED)
target_include_directories(test-server PRIVATE "/usr/local/include")

# 库文件查找路径, 使用自定义路径时用到
link_directories("/usr/local/lib")

# 要是明确链接静态库 or 动态库, 就用 .a 或者 .so, 否则默认是 .so
target_link_libraries(test-server "event_core.a" "event_extra.a" "event_pthreads.a" "pthread" "protobuf.a")

# 日志库 https://github.com/gabime/spdlog, 默认装在 /usr/local/lib64 目录中
target_link_libraries(test-server "spdlog.a")

# 配置文件库 https://github.com/benhoyt/inih
target_link_libraries(test-server "event_core.a")

# 动态库链接时用到, 增加程序动态库查找路径
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

# 一些编译选项
target_compile_options(test-server PRIVATE -fpermissive -std=c++11)

# # OpenSSL::SSL
# message(STATUS "======================      ${LIB_APPS}")
# # ws2_32;shell32;advapi32;bcrypt
# message(STATUS "----------------------      ${LIB_PLATFORM}")