cmake_minimum_required(VERSION 3.20.0)
project(shmipc VERSION 0.1.0 LANGUAGES C CXX)

find_package(assimp REQUIRED)
message("${ASSIMP_LIBRARIES}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
add_library(shmipc src/shmipc.cpp)

add_executable(main src/main.cpp)

add_executable(publisher src/publisher.cpp)
target_link_libraries(publisher shmipc)

add_executable(subscriber src/subscriber.cpp)
target_link_libraries(subscriber shmipc)




set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
