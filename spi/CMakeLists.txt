cmake_minimum_required(VERSION 3.10)

# 项目名称
project(spiflash VERSION 1.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)

# 设置编译器标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")

# 指定编译器
#set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc) # for Luckfox Pico Max, Ubuntu system, refer to https://wiki.luckfox.com/zh/Luckfox-Pico/Luckfox-Pico-RV1106/Luckfox-Pico-Pro-Max/Luckfox-Pico-quick-start
#set(CMAKE_C_COMPILER /opt/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc) # for Luckfox Pico Max, Buildroot system, refer to https://wiki.luckfox.com/zh/Luckfox-Pico/Luckfox-Pico-RV1106/Luckfox-Pico-Pro-Max/Luckfox-Pico-quick-start
#set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc) # for RaspberryPi 4B

# 自动查找项目中的所有 .c 文件（含子目录）并排除 CMakeFiles 目录
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/*.c)
list(FILTER SOURCES EXCLUDE REGEX "CMakeFiles/")

# 自动查找项目中的所有 .h 文件（含子目录）并排除 CMakeFiles 目录
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/*.h)
list(FILTER HEADERS EXCLUDE REGEX "CMakeFiles/")

# 添加所有的源文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 包含所有头文件路径
include_directories(${CMAKE_SOURCE_DIR})

# 如果需要链接库，像 libsqlite3
find_library(SQLITE3_LIB sqlite3)
if(SQLITE3_LIB)
    target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIB})
else()
    message(WARNING "SQLite3 library not found")
endif()

# 设置安装路径（可选）
# install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin)