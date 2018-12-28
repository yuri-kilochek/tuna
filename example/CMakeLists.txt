cmake_minimum_required(VERSION 3.8)

project(example)

add_executable(example
    main.cpp    
)

find_package(tuna REQUIRED)
target_link_libraries(example PRIVATE
    tuna::tuna
)
