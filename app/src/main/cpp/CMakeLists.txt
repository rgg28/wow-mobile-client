cmake_minimum_required(VERSION 3.22.1)
project("wowclient")

add_library(wowclient SHARED
            main.cpp)

find_library(log-lib log)

target_link_libraries(wowclient ${log-lib})
