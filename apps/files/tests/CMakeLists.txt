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
