cmake_minimum_required(VERSION 3.8)

project(example)

add_executable(example
    main.cpp    
)

set_target_properties(example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

find_package(Boost REQUIRED)
target_link_libraries(example PRIVATE
    Boost::boost
)

find_package(tuna REQUIRED)
target_link_libraries(example PRIVATE
    tuna::tuna
)
