cmake_minimum_required(VERSION 3.10)
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-mingw-dynamic")
endif()
# 项目名称
project(StockServer)
# 找到已安装的依赖包
set(CMAKE_CXX_STANDARD 17)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
if(WIN32)
    find_package(unofficial-sqlite3 CONFIG REQUIRED)
else()
    find_package(SQLite3 REQUIRED)
endif()
find_package(Crow CONFIG REQUIRED)
find_package(CURL REQUIRED)
# 源文件列表（现在只有 main.cpp）
set(SOURCES
    main.cpp
    dao/user_dao.cpp
    dao/account_dao.cpp
    dao/position_dao.cpp
    dao/stock_dao.cpp
    dao/news_dao.cpp
    dao/prediction_dao.cpp
    controller/user_ctrl.cpp
    controller/asset_ctrl.cpp
    controller/stock_ctrl.cpp
    utils/http_client.cpp
    service/stock_fetcher.cpp
    service/news_fetcher.cpp
    service/kline_fetcher.cpp
    service/predictor.cpp
    controller/ai_ctrl.cpp
    controller/predict_ctrl.cpp
)
# 生成可执行文件
add_executable(StockServer ${SOURCES})
target_include_directories(StockServer PRIVATE ${CMAKE_SOURCE_DIR})
# 链接库
if(WIN32)
    target_link_libraries(StockServer PRIVATE unofficial::sqlite3::sqlite3)
else()
    target_link_libraries(StockServer PRIVATE SQLite::SQLite3)
endif()

target_link_libraries(StockServer PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    Crow::Crow
)

if(WIN32)
    target_link_libraries(StockServer PRIVATE ws2_32 wsock32 wininet)
else()
    target_link_libraries(StockServer PRIVATE CURL::libcurl pthread)
endif()