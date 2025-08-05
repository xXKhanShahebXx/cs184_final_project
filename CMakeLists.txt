cmake_minimum_required(VERSION 3.10)
project(ClothSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(FREEGLUT_DIR "C:/Users/Faiaz Khan/Downloads/freeglut-MSVC-3.0.0-2.mp/afreeglut")
include_directories(${FREEGLUT_DIR}/include)
link_directories(${FREEGLUT_DIR}/lib/x64)

add_executable(ClothSimulation 
    src/main_visual.cpp
    src/Cloth.cpp
)

if(WIN32)
    find_library(FREEGLUT_STATIC_LIB freeglut_static)
    if(FREEGLUT_STATIC_LIB)
        target_link_libraries(ClothSimulation opengl32 ${FREEGLUT_STATIC_LIB})
    else()
        target_link_libraries(ClothSimulation opengl32 freeglut)
    endif()
endif() 