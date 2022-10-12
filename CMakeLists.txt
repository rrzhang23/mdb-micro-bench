cmake_minimum_required(VERSION 2.8)
project(Dbx1000)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET (CMAKE_C_COMPILER "gcc")
SET (CMAKE_CXX_COMPILER "g++")
SET (CMAKE_CXX_FLAGS "-std=c++11 -Wno-deprecated-declarations -g" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE INTERNAL "compiler options" FORCE)

add_definitions(-DNOGRAPHITE=1)

# include header files
include_directories(
 /home/zhangrongrong/.local/googletest-1.11.0/include
 /home/zhangrongrong/.local/brpc-env/gflags-v2.2.2/include
 /home/zhangrongrong/.local/brpc-env/leveldb-1.23/include
 /home/zhangrongrong/.local/brpc-env/protobuf-4.0.x/include
 /home/zhangrongrong/.local/brpc-1.2.0/include)
include_directories(${PROJECT_SOURCE_DIR}/include)
# lib files
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/libs)
link_directories(
 /home/zhangrongrong/.local/googletest-1.11.0/lib64
 /home/zhangrongrong/.local/brpc-env/gflags-v2.2.2/lib
 /home/zhangrongrong/.local/brpc-env/leveldb-1.23/lib64
 /home/zhangrongrong/.local/brpc-env/protobuf-4.0.x/lib
 /home/zhangrongrong/.local/openssl-1.0.2k/lib
 /home/zhangrongrong/.local/brpc-1.2.0/lib64
 )



 
 
 ### 最保险的做法，该命令会立即被调用。
 execute_process(COMMAND protoc
         -I ${CMAKE_SOURCE_DIR}
         --cpp_out=${CMAKE_SOURCE_DIR}
         ${CMAKE_SOURCE_DIR}/lock_service.proto)



 add_executable(client client.cpp instance.cpp lock_server.cpp lock_service.cpp lock_service.pb.cc)
 target_link_libraries(client pthread gflags leveldb protobuf ssl brpc)

 add_executable(server server.cpp instance.cpp lock_server.cpp lock_service.cpp lock_service.pb.cc)
 target_link_libraries(server pthread gflags leveldb protobuf ssl brpc)