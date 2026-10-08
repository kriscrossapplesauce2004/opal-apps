find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(tst_tonal tst_tonal.cpp)
target_link_libraries(tst_tonal PRIVATE OpalMaterial Qt6::Test)
add_test(NAME tonal COMMAND tst_tonal)
