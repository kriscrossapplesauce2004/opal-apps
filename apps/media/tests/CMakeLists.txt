find_package(Qt6 REQUIRED COMPONENTS Test)

# Tests run with a private home, so they never touch the real index, settings or covers.
qt_add_executable(tst_media tst_media.cpp)
target_link_libraries(tst_media PRIVATE opal-media-core Qt6::Test)
add_test(NAME media_core COMMAND tst_media)
set_tests_properties(media_core PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_GB.UTF-8;HOME=${CMAKE_CURRENT_BINARY_DIR}/home;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/data;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/cache;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/config;DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent" TIMEOUT 120)
