cmake_minimum_required(VERSION 3.20)

# 成员 E 独立工作区：SQLite 数据层 + 票务 Service（与 src/ 参考模板分离）
project(TrainTicketMemberE VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_INCLUDE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets)

# 控制台测试：验证 Database 建表与种子数据
add_executable(ticket_test
    tests/database_manual.cpp
    core/database.cpp
)

target_link_libraries(ticket_test PRIVATE Qt6::Core Qt6::Sql)

if (WIN32)
    set_target_properties(ticket_test PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

# GUI 应用程序
set(GUI_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    dialog.h
    dialog.cpp
    dialog.ui
    core/database.cpp
    core/database.h
    services/AuthService.cpp
    services/AuthService.h
    services/TrainService.cpp
    services/TrainService.h
    services/UserService.cpp
    services/UserService.h
    services/OrderService.h
    services/TicketService.h
    models/user.h
    models/train.h
    models/order.h
)

add_executable(ticket_app ${GUI_SOURCES})

target_link_libraries(ticket_app PRIVATE Qt6::Core Qt6::Sql Qt6::Widgets)

# 让 Qt 的 MOC 和 UIC 自动处理 .h / .ui 文件
set_target_properties(ticket_app PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    WIN32_EXECUTABLE TRUE
)
