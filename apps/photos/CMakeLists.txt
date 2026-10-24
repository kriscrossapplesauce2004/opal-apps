set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.6 REQUIRED COMPONENTS Concurrent DBus ShaderTools GuiPrivate)

# Backend library and the Opal.Photos QML module. Static: the app, the render tool and the tests link it directly.
# Thumbnails and favourites come from the Files backend, so both apps share one cache and one Starred list.
qt_add_library(opal-photos-core STATIC)
qt_add_qml_module(opal-photos-core
    URI Opal.Photos
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES
        src/photo.h
        src/exif.h src/exif.cpp
        src/scanner.h src/scanner.cpp
        src/library.h src/library.cpp
        src/photoview.h src/photoview.cpp
        src/albummodel.h src/albummodel.cpp
        src/photodetails.h src/photodetails.cpp
        src/imageio.h src/imageio.cpp
        src/editor.h src/editor.cpp
        src/photoactions.h src/photoactions.cpp
    QML_FILES
        qml/Main.qml
        qml/PhotosView.qml
        qml/Timeline.qml
        qml/PhotoTile.qml
        qml/Scrubber.qml
        qml/AlbumsPage.qml
        qml/Viewer.qml
        qml/DetailsPanel.qml
        qml/Editor.qml
        qml/SelectionBar.qml
        qml/Slideshow.qml
        qml/PhotosSettings.qml
        qml/Toast.qml
    DEPENDENCIES QtQuick QtQuick.Controls QtQuick.Layouts QtQuick.Dialogs Opal.Material
)
qt_add_shaders(opal-photos-core "photos_shaders"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX /opal/photos
    FILES shaders/adjust.frag)
set_target_properties(opal-photos-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(opal-photos-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(opal-photos-core PUBLIC opal-files-core Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Concurrent Qt6::DBus)

qt_add_executable(opal-photos src/main.cpp)
target_link_libraries(opal-photos PRIVATE opal-photos-core opal-photos-coreplugin Qt6::QuickControls2)
target_compile_definitions(opal-photos PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}")
add_dependencies(opal-photos OpalMaterial OpalMaterialplugin)

# The render tool with the Photos backend linked in, so tools/render.sh can draw the real interface.
qt_add_executable(opal-photos-render ${CMAKE_SOURCE_DIR}/tools/render/main.cpp)
target_link_libraries(opal-photos-render PRIVATE opal-photos-core opal-photos-coreplugin Qt6::GuiPrivate)
target_compile_definitions(opal-photos-render PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}" OPAL_RENDER_INIT_HOOK=opalPhotosRenderInit)
target_sources(opal-photos-render PRIVATE src/renderhook.cpp)
add_dependencies(opal-photos-render OpalMaterial OpalMaterialplugin)

if(OPAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS opal-photos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/dev.opal.Photos.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
