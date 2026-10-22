set(photos_proto ${CMAKE_SOURCE_DIR}/prototypes/photos)
file(GLOB photos_qml CONFIGURE_DEPENDS ${photos_proto}/*.qml ${photos_proto}/*.js)
foreach(file IN LISTS photos_qml)
    get_filename_component(alias ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

qt_add_executable(opal-photos ../prototype_main.cpp)
qt_add_resources(opal-photos photos_ui PREFIX /photos FILES ${photos_qml})
target_link_libraries(opal-photos PRIVATE OpalMaterial Qt6::Quick)
target_compile_definitions(opal-photos PRIVATE
    OPAL_APP_NAME="Photos"
    OPAL_DESKTOP_ID="dev.opal.Photos"
    OPAL_POINTER_QML="qrc:/photos/DesktopTimeline.qml"
    OPAL_TOUCH_QML="qrc:/photos/PhoneGrid.qml"
    OPAL_COUCH_QML="qrc:/photos/Couch.qml"
    OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}"
)
add_dependencies(opal-photos OpalMaterialplugin)

include(GNUInstallDirs)
install(TARGETS opal-photos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/dev.opal.Photos.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
