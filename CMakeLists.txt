cmake_minimum_required(VERSION 3.0)
project(kit_server)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")


include_directories(.)

 # include_directories(/usr/local/yaml-cpp/include)
 # include_directories(/home/nmoek/下载/yaml-cpp-yaml-cpp-0.7.0/build/lib)   


set(LIB_SRC
    kit_server/address.cpp
    kit_server/application.cpp
    kit_server/bytearray.cpp
    kit_server/coroutine.cpp
    kit_server/config.cpp
    kit_server/daemon.cpp
    kit_server/env.cpp
    kit_server/fdmanager.cpp
    kit_server/http/http.cpp
    kit_server/http/http_connection.cpp
    kit_server/http/http_parser.cpp
    kit_server/http/http_session.cpp
    kit_server/http/http_server.cpp
    kit_server/http/servlet.cpp
    kit_server/hook.cpp
    kit_server/iomanager.cpp
    kit_server/Log.cpp
    kit_server/mutex.cpp
    kit_server/scheduler.cpp
    kit_server/socket.cpp
    kit_server/stream.cpp
    kit_server/socket_stream.cpp
    kit_server/tcp_server.cpp
    kit_server/thread.cpp
    kit_server/timer.cpp
    kit_server/util.cpp
)

ragelmaker(/kit_server/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/kit_server/http)

ragelmaker(/kit_server/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/kit_server/http)

ragelmaker(/kit_server/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/kit_server)


add_library(kit_server SHARED ${LIB_SRC})
redefine_file_macro(kit_server)  # redefine __FILE__

SET(LIB_LIB
    kit_server
    dl
    pthread
    yaml-cpp

)


# **********************************tests**********************************#
# 日志系统测试文件 test
add_executable(test tests/test.cpp)
add_dependencies(test kit_server)
redefine_file_macro(test)
target_link_libraries(test ${LIB_LIB})

# 配置系统测试文件 test_config
add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config kit_server)
redefine_file_macro(test_config)
target_link_libraries(test_config ${LIB_LIB})

# 线程库测试文件 test_thread
add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread kit_server)
redefine_file_macro(test_thread)
target_link_libraries(test_thread ${LIB_LIB})

# 通用工具测试文件 test_utils
add_executable(test_utils tests/test_utils.cpp)
add_dependencies(test_utils kit_server)
redefine_file_macro(test_utils)
target_link_libraries(test_utils ${LIB_LIB})

# 协程测试文件 test_coroutine
add_executable(test_coroutine tests/test_coroutine.cpp)
add_dependencies(test_coroutine kit_server)
redefine_file_macro(test_coroutine)
target_link_libraries(test_coroutine ${LIB_LIB})

# 调度器测试文件 test_schedule
add_executable(test_schedule tests/test_schedule.cpp)
add_dependencies(test_schedule kit_server)
redefine_file_macro(test_schedule)
target_link_libraries(test_schedule ${LIB_LIB})


# IO调度测试文件 test_iomanager
add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager kit_server)
redefine_file_macro(test_iomanager)
target_link_libraries(test_iomanager ${LIB_LIB})

# Timer定时器测试文件 test_timer
add_executable(test_timer tests/test_timer.cpp)
add_dependencies(test_timer kit_server)
redefine_file_macro(test_timer)
target_link_libraries(test_timer ${LIB_LIB})

# HOOK测试文件 test_hook
add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook kit_server)
redefine_file_macro(test_hook)
target_link_libraries(test_hook ${LIB_LIB})

# 网络地址测试文件 test_addr
add_executable(test_addr tests/test_addr.cpp)
add_dependencies(test_addr kit_server)
redefine_file_macro(test_addr)
target_link_libraries(test_addr ${LIB_LIB})

# socket API 测试文件 test_socket
add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket kit_server)
redefine_file_macro(test_socket)
target_link_libraries(test_socket ${LIB_LIB})

# 数据序列化 测试文件 test_bytearray
add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray kit_server)
redefine_file_macro(test_bytearray)
target_link_libraries(test_bytearray ${LIB_LIB})

# HTTP协议封装 测试文件 test_http
add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http kit_server)
redefine_file_macro(test_http)
target_link_libraries(test_http ${LIB_LIB})

# HTTP协议解析 测试文件 test_http_parser
add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser kit_server)
redefine_file_macro(test_http_parser)
target_link_libraries(test_http_parser ${LIB_LIB})

# Tcp Server 测试文件 test_tcp_server
add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server kit_server)
redefine_file_macro(test_tcp_server)
target_link_libraries(test_tcp_server ${LIB_LIB})

# HTTP Server 测试文件 test_http_server
add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server kit_server)
redefine_file_macro(test_http_server)
target_link_libraries(test_http_server ${LIB_LIB})

# HTTP Connection 测试文件 test_http_connection
add_executable(test_http_connection tests/test_http_connection.cpp)
add_dependencies(test_http_connection kit_server)
redefine_file_macro(test_http_connection)
target_link_libraries(test_http_connection ${LIB_LIB})

# URI 测试文件 test_uri
add_executable(test_uri tests/test_uri.cpp)
add_dependencies(test_uri kit_server)
redefine_file_macro(test_uri)
target_link_libraries(test_uri ${LIB_LIB})


# daemon 测试文件 test_daemon
add_executable(test_daemon tests/test_daemon.cpp)
add_dependencies(test_daemon kit_server)
redefine_file_macro(test_daemon)
target_link_libraries(test_daemon ${LIB_LIB})


# env 测试文件 test_env
add_executable(test_env tests/test_env.cpp)
add_dependencies(test_env kit_server)
redefine_file_macro(test_env)
target_link_libraries(test_env ${LIB_LIB})

# application 测试文件 test_application
add_executable(test_application tests/test_application.cpp)
add_dependencies(test_application kit_server)
redefine_file_macro(test_application)
target_link_libraries(test_application ${LIB_LIB})

# **********************************example**********************************#
# Tcp Server 实例文件 echo_server
add_executable(echo_server example/echo_server.cpp)
add_dependencies(echo_server kit_server)
redefine_file_macro(echo_server)
target_link_libraries(echo_server ${LIB_LIB})

# **********************************压测例子**********************************#
add_executable(my_http_server ab_example/my_http_server.cpp)
add_dependencies(my_http_server kit_server)
redefine_file_macro(my_http_server)
target_link_libraries(my_http_server ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)