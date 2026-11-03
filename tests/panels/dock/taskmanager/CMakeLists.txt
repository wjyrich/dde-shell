# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Test)

include(GoogleTest)

add_executable(rolecombinemodel_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolecombinemodel.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolecombinemodel.cpp
    combinemodela.cpp
    combinemodela.h
    combinemodelb.cpp
    combinemodelb.h
    rolecombinemodeltests.cpp
)

target_link_libraries(rolecombinemodel_tests
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)
target_include_directories(rolecombinemodel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/
)

gtest_discover_tests(rolecombinemodel_tests)

add_executable(rolegroupmodel_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolegroupmodel.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/rolegroupmodel.cpp
    rolegroupmodeltests.cpp
)

target_link_libraries(rolegroupmodel_tests
    GTest::GTest
    GTest::Main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    # dock-taskmanager
)
target_include_directories(rolegroupmodel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/
)

gtest_discover_tests(rolegroupmodel_tests)

add_executable(taskoverflowcontroller_tests
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/taskoverflowcontroller.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/taskoverflowcontroller.h
    taskoverflowcontrollertests.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/textcalculator.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/textcalculator.h
)
target_include_directories(taskoverflowcontroller_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager
)
target_link_libraries(taskoverflowcontroller_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Test
)
target_link_libraries(taskoverflowcontroller_tests PRIVATE Qt${QT_VERSION_MAJOR}::Quick)
target_compile_definitions(taskoverflowcontroller_tests PRIVATE
    TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    TEST_BUILD_DIR="${CMAKE_BINARY_DIR}"
    TEST_FRAME_LIBRARY="$<TARGET_FILE:dde-shell-frame>"
)
add_dependencies(taskoverflowcontroller_tests dock-plugin dde-shell-plugin)
add_test(NAME taskoverflowcontroller_tests COMMAND taskoverflowcontroller_tests)
set_tests_properties(taskoverflowcontroller_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
