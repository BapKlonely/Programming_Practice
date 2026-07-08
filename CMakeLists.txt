cmake_minimum_required(VERSION 3.20)

project(TrainTicketSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(TrainTicketSystem
    src/main.cpp
    src/customtitlebar.h
    src/customtitlebar.cpp
    src/style.h
    src/style.cpp
    src/logindialog.h
    src/logindialog.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/datastore.h
    src/datastore.cpp
)

target_link_libraries(TrainTicketSystem PRIVATE Qt6::Widgets Qt6::Sql)

if (WIN32)
    set_target_properties(TrainTicketSystem PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
