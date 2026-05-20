cmake_minimum_required(VERSION 3.20)
project(game)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6.1 QUIET COMPONENTS graphics window system)

if (NOT SFML_FOUND)
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SFML)
endif ()

add_executable(game
    main.cpp
    src/Animation.cpp
    src/AssetStore.cpp
    src/Bullet.cpp
    src/Enemy.cpp
    src/Entity.cpp
    src/Game.cpp
    src/Platform.cpp
    src/Player.cpp
)

target_include_directories(game PRIVATE include)
target_link_libraries(game PRIVATE sfml-graphics sfml-window sfml-system winmm)
