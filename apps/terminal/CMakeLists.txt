# Terminal engine (libvterm + scene-graph renderer) and the minimal app shell.
enable_language(C)
find_package(Qt6 REQUIRED COMPONENTS ShaderTools)

set(VTERM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libvterm)
add_library(vterm STATIC
    ${VTERM_DIR}/src/encoding.c ${VTERM_DIR}/src/keyboard.c ${VTERM_DIR}/src/mouse.c ${VTERM_DIR}/src/parser.c
    ${VTERM_DIR}/src/pen.c ${VTERM_DIR}/src/screen.c ${VTERM_DIR}/src/state.c ${VTERM_DIR}/src/unicode.c ${VTERM_DIR}/src/vterm.c)
set_target_properties(vterm PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON)
target_include_directories(vterm PUBLIC ${VTERM_DIR}/include PRIVATE ${VTERM_DIR}/src)
target_compile_options(vterm PRIVATE -w)

qt_add_library(OpalTerminalEngine SHARED)
qt_add_qml_module(OpalTerminalEngine
    URI Opal.Terminal
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES
        src/vt/terminalpty.h src/vt/terminalpty.cpp
        src/vt/terminalpalette.h src/vt/terminalpalette.cpp
        src/vt/commandmarksmodel.h src/vt/commandmarksmodel.cpp
        src/vt/terminalsession.h src/vt/terminalsession.cpp
        src/vt/keytranslation.h src/vt/keytranslation.cpp
        src/vt/glyphatlas.h src/vt/glyphatlas.cpp
        src/vt/terminalitem.h src/vt/terminalitem.cpp
    DEPENDENCIES QtQuick
)
qt_add_shaders(OpalTerminalEngine "terminal_shaders"
    BATCHABLE
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/src/vt
    PREFIX /opal/terminal
    FILES src/vt/shaders/cells.vert src/vt/shaders/cells.frag
    OUTPUT_TARGETS shader_targets)
target_include_directories(OpalTerminalEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vt)
target_link_libraries(OpalTerminalEngine PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick PRIVATE vterm)
target_compile_definitions(OpalTerminalEngine PRIVATE OPAL_TERMINAL_LIBRARY)

# The app interface as its own module, so tests and the render tool can load it.
qt_add_library(OpalTerminalUi SHARED)
qt_add_qml_module(OpalTerminalUi
    URI Opal.TerminalUi
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    SOURCES
        src/ui/terminalconfig.h src/ui/terminalconfig.cpp
        src/ui/terminalthemes.h src/ui/terminalthemes.cpp
        src/ui/terminalhelpers.h src/ui/terminalhelpers.cpp
    QML_FILES
        qml/ui/TerminalView.qml qml/ui/TermPane.qml qml/ui/SplitNode.qml qml/ui/TermTabs.qml qml/ui/SessionsSidebar.qml
        qml/ui/CommandPalette.qml qml/ui/AppearancePanel.qml qml/ui/KeyRow.qml qml/ui/SessionEntry.qml
    DEPENDENCIES QtQuick Opal.Material Opal.Terminal
)
qt_add_shaders(OpalTerminalUi "terminal_ui_shaders"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/qml/ui
    PREFIX /opal/terminal-ui
    FILES qml/ui/shaders/crt.frag qml/ui/shaders/bloom.frag qml/ui/shaders/edgeglow.frag)
target_include_directories(OpalTerminalUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
target_link_libraries(OpalTerminalUi PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick OpalTerminalEngine)
add_dependencies(OpalTerminalUi OpalMaterialplugin OpalTerminalEngineplugin)

qt_add_executable(opal-terminal src/main.cpp)
qt_add_qml_module(opal-terminal
    URI OpalTerminalApp
    VERSION 1.0
    QML_FILES qml/Main.qml
    RESOURCE_PREFIX /qt/qml
)
target_link_libraries(opal-terminal PRIVATE OpalTerminalEngine OpalTerminalUi OpalMaterial Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(opal-terminal PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}")
add_dependencies(opal-terminal OpalMaterialplugin OpalTerminalEngineplugin OpalTerminalUiplugin)

include(GNUInstallDirs)
install(TARGETS opal-terminal OpalTerminalEngine OpalTerminalEngineplugin OpalTerminalUi OpalTerminalUiplugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/dev.opal.Terminal.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(OPAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()
