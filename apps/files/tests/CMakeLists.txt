find_package(Qt6 REQUIRED COMPONENTS Test)

# Every test runs with a private home so it never touches the real starred list, thumbnails or trash.
function(opal_files_test name)
    qt_add_executable(tst_${name} tst_${name}.cpp)
    target_link_libraries(tst_${name} PRIVATE opal-files-core Qt6::Test)
    add_test(NAME files_${name} COMMAND tst_${name})
    set_tests_properties(files_${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_GB.UTF-8;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/data;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/cache;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/config" TIMEOUT 180)
endfunction()

opal_files_test(foldermodel)
opal_files_test(fileops)
opal_files_test(places)
opal_files_test(search)
opal_files_test(thumbnailer)

# Interface test: loads the real FilesView offscreen and drives it.
find_package(Qt6 REQUIRED COMPONENTS QuickTest)
qt_add_executable(tst_filesui tst_filesui.cpp)
target_link_libraries(tst_filesui PRIVATE opal-files-core opal-files-coreplugin Qt6::QuickTest Qt6::Quick)
target_compile_definitions(tst_filesui PRIVATE OPAL_QML_DIR="${QT_QML_OUTPUT_DIRECTORY}" QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
add_dependencies(tst_filesui OpalMaterial OpalMaterialplugin)
add_test(NAME files_ui COMMAND tst_filesui)
set_tests_properties(files_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_GB.UTF-8;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/data;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/cache;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/config;OPAL_NO_GAMEPAD=1;OPAL_POSTURE=pointer" TIMEOUT 120)
