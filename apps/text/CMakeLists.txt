qt_add_executable(opal-text-editor ../prototype_main.cpp)
qt_add_resources(opal-text-editor text_ui
    PREFIX /text
    FILES qml/Main.qml
)
target_link_libraries(opal-text-editor PRIVATE OpalMaterial Qt6::Quick)
target_compile_definitions(opal-text-editor PRIVATE
    OPAL_APP_NAME="Text Editor"
    OPAL_DESKTOP_ID="dev.opal.TextEditor"
    OPAL_POINTER_QML="qrc:/text/qml/Main.qml"
    OPAL_TOUCH_QML="qrc:/text/qml/Main.qml"
    OPAL_COUCH_QML="qrc:/text/qml/Main.qml"
    OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}"
)
add_dependencies(opal-text-editor OpalMaterialplugin)

include(GNUInstallDirs)
install(TARGETS opal-text-editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
