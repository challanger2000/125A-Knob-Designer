cmake_minimum_required(VERSION 3.20)
project(125A_Knob_Designer VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "125A Knob Designer v0.1.0 currently targets Windows only.")
endif()

add_executable(125A_Knob_Designer
    src/main.cpp
    src/KnobRenderer.cpp
    src/KnobRenderer.h
)

target_compile_features(125A_Knob_Designer PRIVATE cxx_std_17)
target_compile_definitions(125A_Knob_Designer PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(125A_Knob_Designer PRIVATE gdiplus)

if(MSVC)
    target_compile_options(125A_Knob_Designer PRIVATE /W4 /permissive-)
endif()
