project(settings)

file(GLOB ${PROJECT_NAME}_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h	
)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_sources})
