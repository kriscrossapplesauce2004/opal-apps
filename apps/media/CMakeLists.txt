set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.6 REQUIRED COMPONENTS Concurrent DBus Multimedia Network GuiPrivate)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)

# Backend library and the Opal.Media QML module. Static: the app, the render tool and the tests link it directly.
qt_add_library(opal-media-core STATIC)
qt_add_qml_module(opal-media-core
    URI Opal.Media
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES
        src/track.h
        src/medialibrary.h src/medialibrary.cpp
        src/player.h src/player.cpp
        src/mpris.h src/mpris.cpp
        src/radio.h src/radio.cpp
    QML_FILES
        qml/Main.qml
        qml/MediaView.qml
        qml/HomePage.qml
        qml/MusicPage.qml
        qml/CollectionPage.qml
        qml/VideosPage.qml
        qml/VideoPlayer.qml
        qml/RadioPage.qml
        qml/BooksPage.qml
        qml/SearchPage.qml
        qml/NowPlayingPanel.qml
        qml/FullPlayer.qml
        qml/MiniPlayer.qml
        qml/Transport.qml
        qml/PlayButton.qml
        qml/Cover.qml
        qml/SplitPill.qml
        qml/QueueList.qml
        qml/LyricsView.qml
        qml/TrackRow.qml
        qml/MediaSettings.qml
        qml/Toast.qml
    DEPENDENCIES QtQuick QtQuick.Controls QtQuick.Layouts QtQuick.Dialogs QtMultimedia Opal.Material
)
set_target_properties(opal-media-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(opal-media-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(opal-media-core PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Concurrent Qt6::DBus Qt6::Multimedia Qt6::Network PkgConfig::TAGLIB)

qt_add_executable(opal-media src/main.cpp)
target_link_libraries(opal-media PRIVATE opal-media-core opal-media-coreplugin Qt6::QuickControls2)
target_compile_definitions(opal-media PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}")
add_dependencies(opal-media OpalMaterial OpalMaterialplugin)

# The render tool with the Media backend linked in, so tools/render.sh can draw the real interface.
qt_add_executable(opal-media-render ${CMAKE_SOURCE_DIR}/tools/render/main.cpp)
target_link_libraries(opal-media-render PRIVATE opal-media-core opal-media-coreplugin Qt6::GuiPrivate)
target_compile_definitions(opal-media-render PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}")
add_dependencies(opal-media-render OpalMaterial OpalMaterialplugin)

if(OPAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS opal-media RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/dev.opal.Media.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
