find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(tst_terminal tst_terminal.cpp)
target_link_libraries(tst_terminal PRIVATE OpalTerminalEngine Qt6::Test)
add_test(NAME terminal COMMAND tst_terminal)
