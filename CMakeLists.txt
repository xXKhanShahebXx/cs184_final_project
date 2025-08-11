cmake_minimum_required(VERSION 3.10)
project(ClothSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(FREEGLUT_DIR "${CMAKE_SOURCE_DIR}/freeglut")
include_directories(${FREEGLUT_DIR}/include)
link_directories(${FREEGLUT_DIR}/lib/x64)

add_executable(ClothSimulation 
    src/main_visual.cpp
    src/Cloth.cpp
    src/Water.cpp
    src/Coupling.cpp
    src/WaterRenderer.cpp
)

if(WIN32)
    find_library(FREEGLUT_STATIC_LIB freeglut_static)
    if(FREEGLUT_STATIC_LIB)
        target_link_libraries(ClothSimulation opengl32 ${FREEGLUT_STATIC_LIB})
    else()
        target_link_libraries(ClothSimulation opengl32 freeglut)
    endif()
    # Link GLEW (assumes it's available via system or vcpkg toolchain)
    find_package(GLEW QUIET)
    if(GLEW_FOUND)
        target_link_libraries(ClothSimulation GLEW::GLEW)
    else()
        # Fallback: try to link glew32
        target_link_libraries(ClothSimulation glew32)
    endif()

    set(FREEGLUT_DLL_CANDIDATES
        "${FREEGLUT_DIR}/bin/freeglut.dll"
        "${FREEGLUT_DIR}/bin/x64/freeglut.dll"
    )
    foreach(DLL_CANDIDATE ${FREEGLUT_DLL_CANDIDATES})
        if(EXISTS "${DLL_CANDIDATE}")
            add_custom_command(TARGET ClothSimulation POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_CANDIDATE}"
                    "$<TARGET_FILE_DIR:ClothSimulation>/freeglut.dll"
            )
        endif()
    endforeach()
endif() 