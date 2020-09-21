cmake_minimum_required(VERSION 3.4.1)

add_library(platform SHARED src/main/cpp/platform.cpp)

find_library(log-lib log)
find_library(android-lib android)
target_link_libraries(platform ${android-lib} ${log-lib})
