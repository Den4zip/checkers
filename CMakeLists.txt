# Указываем минимально необходимую версию CMake
cmake_minimum_required(VERSION 3.10)

# Задаем имя проекта и язык
project(checkers LANGUAGES CXX)

# Ищем библиотеку ncurses
find_package(Curses REQUIRED)

# Добавляем исполняемый файл и указываем исходные файлы
add_executable(checkers checkers.cpp)

# === НОВАЯ СТРОКА ===
# Говорим компилятору, что мы линкуемся со статической библиотекой ncurses
target_compile_definitions(checkers PRIVATE NCURSES_STATIC)

# Присоединяем библиотеку ncurses к нашему исполняемому файлу
target_link_libraries(checkers PRIVATE ${CURSES_LIBRARIES})

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)