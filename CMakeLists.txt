cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(GameServer Core/main.cpp
        Core/NetworkManager.cpp
        Core/NetworkManager.h
        Core/GameMessage.h
        Core/GameServer.cpp
        Core/GameServer.h
        Core/Entity.cpp
        Core/Entity.h
)

target_compile_features(GameServer PRIVATE cxx_std_17)
target_link_libraries(GameServer PRIVATE SFML::Network)