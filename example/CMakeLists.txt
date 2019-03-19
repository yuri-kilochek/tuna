cmake_minimum_required(VERSION 3.8)

project(example)

add_executable(example
    main.cpp    
)

target_compile_features(example PRIVATE cxx_std_11)

find_package(tuna REQUIRED)
find_package(boost REQUIRED)
target_link_libraries(example PRIVATE
    tuna::tuna
    boost::boost
)
