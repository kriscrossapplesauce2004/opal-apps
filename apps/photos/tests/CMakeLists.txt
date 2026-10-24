find_package(Qt6 REQUIRED COMPONENTS Test)

# Tests run with a private home, so they never touch the real index, favourites or thumbnails.
qt_add_executable(tst_photos tst_photos.cpp)
target_link_libraries(tst_photos PRIVATE opal-photos-core Qt6::Test)
add_test(NAME photos_core COMMAND tst_photos)
set_tests_properties(photos_core PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_GB.UTF-8;OPAL_PHOTOS_NO_OCR=1;HOME=${CMAKE_CURRENT_BINARY_DIR}/home;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/data;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/cache;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/xdg/config" TIMEOUT 120)
