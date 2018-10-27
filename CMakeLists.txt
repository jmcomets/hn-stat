project(hnStat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -g")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-omit-frame-pointer")

add_executable(hnStat
    src/main.cpp
    src/timestamp.cpp
    src/tsv_reader.cpp
    src/options.cpp
    )
