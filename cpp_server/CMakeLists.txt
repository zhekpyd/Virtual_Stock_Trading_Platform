cmake_minimum_required(VERSION 3.10)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-mingw-dynamic")
# 项目名称
project(StockServer)
# 找到已安装的依赖包
set(CMAKE_CXX_STANDARD 17)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(CURL REQUIRED)
# 源文件列表（现在只有 main.cpp）
set(SOURCES
    main.cpp
    dao/user_dao.cpp
    dao/account_dao.cpp
    dao/position_dao.cpp
    dao/stock_dao.cpp
    controller/user_ctrl.cpp
    controller/asset_ctrl.cpp
    controller/stock_ctrl.cpp
    utils/http_client.cpp
    service/stock_fetcher.cpp
)
# 生成可执行文件
add_executable(StockServer ${SOURCES})
target_include_directories(StockServer PRIVATE ${CMAKE_SOURCE_DIR})
target_link_options(StockServer PRIVATE -lws2_32 -lwsock32)
# 链接库
target_link_libraries(StockServer PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    Crow::Crow
    ws2_32
    wsock32
    unofficial::sqlite3::sqlite3
    winhttp
)