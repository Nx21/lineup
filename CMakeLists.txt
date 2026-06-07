cmake_minimum_required(VERSION 3.20)
project(WorldCupAnalyst VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Gui Svg)

qt_standard_project_setup()

set(APP_SOURCES
    src/main.cpp
    src/DemoData.h
    src/api/ApiModels.h
    src/api/ApiClient.h
    src/api/ApiClient.cpp
    src/db/DatabaseManager.h
    src/db/DatabaseManager.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/TacticalPitchView.h
    src/ui/TacticalPitchView.cpp
    src/ui/PlayerToken.h
    src/ui/PlayerToken.cpp
    src/ui/PlayerRosterWidget.h
    src/ui/PlayerRosterWidget.cpp
    src/ui/SuggestionPanel.h
    src/ui/SuggestionPanel.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/engine/LineupSuggestionEngine.h
    src/engine/LineupSuggestionEngine.cpp
    resources/resources.qrc
)

qt_add_executable(WorldCupAnalyst ${APP_SOURCES})

target_include_directories(WorldCupAnalyst PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(WorldCupAnalyst PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Gui
    Qt6::Svg
)

set_target_properties(WorldCupAnalyst PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# ── Tests ────────────────────────────────────────────────────────────────────
find_package(Qt6 COMPONENTS Test QUIET)
if(TARGET Qt6::Test)
    enable_testing()

    qt_add_executable(test_suggestion_engine
        tests/test_suggestion_engine.cpp
        src/engine/LineupSuggestionEngine.h
        src/engine/LineupSuggestionEngine.cpp
        src/api/ApiModels.h
    )

    target_include_directories(test_suggestion_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(test_suggestion_engine PRIVATE
        Qt6::Core
        Qt6::Test
    )

    add_test(NAME SuggestionEngineTest COMMAND test_suggestion_engine)
endif()
