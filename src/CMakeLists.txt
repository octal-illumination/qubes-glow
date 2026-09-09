cmake_minimum_required(VERSION 3.16)
project(kittyglow VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets DBus)
find_package(KF5 REQUIRED COMPONENTS WindowSystem CoreAddons Config GlobalAccel)
find_package(epoxy REQUIRED)

add_definitions(-DQT_NO_KEYWORDS)

# KWin effects headers live in /usr/include (default search path). KWin itself
# already provides kwineffects/kwinglutils/XCB/epoxy at runtime, so we link the
# shared libs directly and avoid the imported targets' dependency chain.
find_library(KWINEFFECTS_LIB NAMES kwineffects REQUIRED)
find_library(KWINGLUTILS_LIB NAMES kwinglutils)

add_library(kittyglow MODULE kittyglow.cpp glowshader.cpp)
set_target_properties(kittyglow PROPERTIES PREFIX "" OUTPUT_NAME "kittyglow")

target_link_libraries(kittyglow
    ${KWINEFFECTS_LIB}
    ${KWINGLUTILS_LIB}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::DBus
    KF5::WindowSystem
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::GlobalAccel
    epoxy::epoxy)
