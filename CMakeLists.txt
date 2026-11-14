cmake_minimum_required(VERSION 3.16)

project(ProjectMine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)

# Configuracao comum (includes, warnings, raylib estatico) para jogo e testes.
# enet e tmxlite estao em lib/ e include/ mas ainda nao sao linkados; quando
# forem usados, adicione aqui:
#   tmxlite
#   enet ws2_32   (e copie lib/libenet-7.dll para junto do exe)
file(RELATIVE_PATH LIB_DIR_REL ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/lib)
function(setup_target target)
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_include_directories(${target} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    # Build de desenvolvimento guarda os dados do usuario em resource/ (ver persistence/data_dir.cpp)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${target} PRIVATE PROJECT_MINE_DEV)
    endif()
    # O caminho do projeto tem acento ("Área") e o ld do MinGW nao abre caminhos
    # absolutos com acento; por isso lib/ entra como -L relativo a pasta de build.
    target_link_options(${target} PRIVATE -L${LIB_DIR_REL})
    target_link_libraries(${target} PRIVATE raylib opengl32 gdi32 winmm)
    # Exe independente das DLLs do MSYS2
    target_link_options(${target} PRIVATE -static)
endfunction()

# --- Jogo -------------------------------------------------------------------
if(GAME_SOURCES)
    add_executable(project-mine ${GAME_SOURCES})
    setup_target(project-mine)
else()
    message(WARNING "Nenhum .cpp em src/ ainda - crie src/main.cpp e rode o cmake de novo.")
endif()

# --- Testes (GoogleTest) ----------------------------------------------------
# GoogleTest instalado pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-gtest
# Compila tests/*.cpp junto com o codigo do jogo, exceto src/main.cpp.
if(TEST_SOURCES)
    enable_testing()
    # Procura na pasta lib/ ao lado do compilador (ex.: C:/msys64/ucrt64/lib)
    get_filename_component(COMPILER_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    find_library(GTEST_LIB NAMES libgtest.a HINTS ${COMPILER_BIN_DIR}/../lib REQUIRED)
    find_library(GTEST_MAIN_LIB NAMES libgtest_main.a HINTS ${COMPILER_BIN_DIR}/../lib REQUIRED)
    set(GAME_SOURCES_NO_MAIN ${GAME_SOURCES})
    list(REMOVE_ITEM GAME_SOURCES_NO_MAIN ${CMAKE_SOURCE_DIR}/src/main.cpp)
    add_executable(project-mine-tests ${TEST_SOURCES} ${GAME_SOURCES_NO_MAIN})
    setup_target(project-mine-tests)
    target_link_libraries(project-mine-tests PRIVATE ${GTEST_MAIN_LIB} ${GTEST_LIB})
    add_test(NAME project-mine-tests COMMAND project-mine-tests)
endif()
