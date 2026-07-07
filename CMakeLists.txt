cmake_minimum_required(VERSION 3.19)
project(train_ticket_system LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(train_ticket_system
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    dialog.h dialog.cpp dialog.ui
    datastore.h datastore.cpp
    src/models/train.h
    src/services/TrainService.h
    src/services/TrainService.cpp
)

target_link_libraries(train_ticket_system
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS train_ticket_system
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET train_ticket_system
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
