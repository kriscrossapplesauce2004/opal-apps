set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.6 REQUIRED COMPONENTS Concurrent DBus GuiPrivate)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)
pkg_check_modules(POPPLER_QT6 IMPORTED_TARGET poppler-qt6)

# Backend library and the Opal.Files QML module. Static: the app and the tests link it directly.
qt_add_library(opal-files-core STATIC)
qt_add_qml_module(opal-files-core
    URI Opal.Files
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES
        src/gobj.h
        src/glibloop.h src/glibloop.cpp
        src/format.h src/format.cpp
        src/starred.h src/starred.cpp
        src/thumbnailer.h src/thumbnailer.cpp
        src/entrymodel.h src/entrymodel.cpp
        src/foldermodel.h src/foldermodel.cpp
        src/places.h src/places.cpp
        src/fileinfo.h src/fileinfo.cpp
        src/filesenv.h
    QML_FILES
        qml/DebugMain.qml
    DEPENDENCIES QtQuick
)
set_target_properties(opal-files-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(opal-files-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(opal-files-core PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Concurrent Qt6::DBus PkgConfig::GIO)
if(POPPLER_QT6_FOUND)
    target_link_libraries(opal-files-core PUBLIC PkgConfig::POPPLER_QT6)
    target_compile_definitions(opal-files-core PUBLIC OPAL_HAVE_POPPLER)
endif()

qt_add_executable(opal-files src/main.cpp)
target_link_libraries(opal-files PRIVATE opal-files-core opal-files-coreplugin Qt6::QuickControls2)
target_compile_definitions(opal-files PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}")
add_dependencies(opal-files OpalMaterial OpalMaterialplugin)

# The render tool with the Files backend linked in, so tools/render.sh can draw the debug UI.
qt_add_executable(opal-files-render ${CMAKE_SOURCE_DIR}/tools/render/main.cpp)
target_link_libraries(opal-files-render PRIVATE opal-files-core opal-files-coreplugin Qt6::GuiPrivate)
target_compile_definitions(opal-files-render PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}" OPAL_RENDER_INIT_HOOK=opalFilesRenderInit)
target_sources(opal-files-render PRIVATE src/renderhook.cpp)
add_dependencies(opal-files-render OpalMaterial OpalMaterialplugin)

if(OPAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS opal-files RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
