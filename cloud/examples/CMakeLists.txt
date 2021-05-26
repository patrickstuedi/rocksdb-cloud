cmake_minimum_required(VERSION 3.8)
project("rocksdb-cloud")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-fPIC -O3 -g -D_GLIBCXX_USE_CXX11_ABI=0")
#set(CMAKE_SHARED_LINKER_FLAGS "-shared")
include_directories ("/usr/local/include")
add_executable(cloud_durable
	cloud_durable_example.cc
	)
target_link_libraries (cloud_durable LINK_PUBLIC rocksdb aws-cpp-sdk-s3 aws-cpp-sdk-kinesis aws-cpp-sdk-core aws-cpp-sdk-transfer pthread rt dl snappy gflags z bz2 lz4 zstd -shared)
install(TARGETS cloud_durable DESTINATION /bin)
