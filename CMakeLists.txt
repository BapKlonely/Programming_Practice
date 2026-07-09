cmake_minimum_required(VERSION 3.20)

# 成员 E 独立工作区：SQLite 数据层 + 票务 Service（与 src/ 参考模板分离）
project(TrainTicketMemberE VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_INCLUDE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# 控制台测试：验证 Database 建表与种子数据
add_executable(ticket_test
    tests/database_manual.cpp
    core/database.cpp
)

target_link_libraries(ticket_test PRIVATE Qt6::Core Qt6::Sql)

if (WIN32)
    set_target_properties(ticket_test PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
