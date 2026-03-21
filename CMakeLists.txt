cmake_minimum_required(VERSION 3.20)

project(middleware VERSION 1.0)

# Find requirements
if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()

# Set C++11
# include(CheckCXXCompilerFlag)
# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
#         CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
#     if(SUPPORTS_CXX11)
#         add_compile_options(-std=c++11)
#     else()
#         message(FATAL_ERROR "Compiler doesn't support C++11")
#     endif()
# endif()

message(STATUS "Configuring Middleware publisher/subscriber example...")
file(GLOB RPC_FASTDDS_SOURCES_CXX "src/rpc/fastdds/*.cxx")

# set(MSG_FILE src/fastdds/ByteArrayPubSubTypes.cxx
#              src/fastdds/ByteArrayObjectSupport.cxx)

add_executable(tool_rpc_ros_pub tool/tool_rpc_ros_pub.cpp src/rpc/fastdds/rpc_ros_pub.cpp ${RPC_FASTDDS_SOURCES_CXX})
target_link_libraries(tool_rpc_ros_pub fastdds fastcdr)
target_include_directories(tool_rpc_ros_pub PUBLIC incl)

add_executable(tool_rpc_ros_sub tool/tool_rpc_ros_sub.cpp src/rpc/fastdds/rpc_ros_sub.cpp ${RPC_FASTDDS_SOURCES_CXX})
target_link_libraries(tool_rpc_ros_sub fastdds fastcdr)
target_include_directories(tool_rpc_ros_sub PUBLIC incl)