cmake_minimum_required(VERSION 3.24)
project(opal-apps VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus)
qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

# All QML modules build into one import tree so the qml tool and the apps find them.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL3 IMPORTED_TARGET sdl3)
endif()

option(OPAL_BUILD_TESTS "Build the Opal.Material tests" ON)

add_subdirectory(material)
add_subdirectory(apps)
add_subdirectory(tools)

if(OPAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
