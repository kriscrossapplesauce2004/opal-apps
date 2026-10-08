qt_add_library(OpalMaterial SHARED)

set(OPAL_MATERIAL_QML
    qml/Theme.qml qml/MText.qml qml/Glyph.qml qml/Surface.qml qml/MButton.qml qml/Chip.qml qml/MorphChip.qml
    qml/ExpressiveSlider.qml qml/SwitchRow.qml qml/ChoiceGroup.qml qml/HintBar.qml qml/FocusRing.qml qml/ListRow.qml
    qml/SearchField.qml qml/NavRail.qml qml/NavBar.qml qml/CouchTabs.qml qml/Fab.qml qml/TopBar.qml qml/Sheet.qml
    qml/Card.qml qml/SegmentedButtons.qml qml/WavyProgress.qml qml/AppFrame.qml qml/AppWindow.qml qml/RoundedImage.qml)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(OpalMaterial
    URI Opal.Material
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES src/tonal.h src/tonal.cpp src/palette.h src/palette.cpp src/posture.h src/posture.cpp src/gamepad.h src/gamepad.cpp
    QML_FILES ${OPAL_MATERIAL_QML}
    DEPENDENCIES QtQuick QtQuick.Controls QtQuick.Layouts QtQuick.Shapes QtQuick.Effects
)

target_include_directories(OpalMaterial PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(OpalMaterial PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 PRIVATE Qt6::DBus)
if(SDL3_FOUND)
    target_link_libraries(OpalMaterial PRIVATE PkgConfig::SDL3)
    target_compile_definitions(OpalMaterial PRIVATE OPAL_HAVE_SDL)
endif()

include(GNUInstallDirs)
install(TARGETS OpalMaterial OpalMaterialplugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${QT_QML_OUTPUT_DIRECTORY}/Opal/Material DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/qml/Opal)
