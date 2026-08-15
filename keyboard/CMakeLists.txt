cmake_minimum_required(VERSION 3.20)
project(moonlight_keyboard_layer LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(LayerShellQt REQUIRED)

add_library(moonlight-keyboard-layer SHARED layer_shell_bridge.cpp)
target_compile_features(moonlight-keyboard-layer PRIVATE cxx_std_17)
target_link_libraries(
    moonlight-keyboard-layer
    PRIVATE
        Qt6::Gui
        LayerShellQt::Interface
)

set_target_properties(
    moonlight-keyboard-layer
    PROPERTIES
        OUTPUT_NAME moonlight-keyboard-layer
        PREFIX lib
)
