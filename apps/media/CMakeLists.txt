set(media_proto ${CMAKE_SOURCE_DIR}/prototypes/media)
file(GLOB media_qml CONFIGURE_DEPENDS ${media_proto}/*.qml ${media_proto}/*.js)
foreach(file IN LISTS media_qml)
    get_filename_component(alias ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

qt_add_executable(opal-media ../prototype_main.cpp)
qt_add_resources(opal-media media_ui PREFIX /media FILES ${media_qml})
target_link_libraries(opal-media PRIVATE OpalMaterial Qt6::Quick)
target_compile_definitions(opal-media PRIVATE
    OPAL_APP_NAME="Media"
    OPAL_DESKTOP_ID="dev.opal.Media"
    OPAL_POINTER_QML="qrc:/media/DesktopLibrary.qml"
    OPAL_TOUCH_QML="qrc:/media/PhoneHome3.qml"
    OPAL_COUCH_QML="qrc:/media/Couch.qml"
    OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}"
)
add_dependencies(opal-media OpalMaterialplugin)

include(GNUInstallDirs)
install(TARGETS opal-media RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
