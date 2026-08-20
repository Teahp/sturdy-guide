cmake_minimum_required(VERSION 3.16)

project(SturdyGuideConsumer LANGUAGES CXX)

# 这是独立项目：它从 CMAKE_PREFIX_PATH 查找已安装 SDK，不读取源码目录。
find_package(SturdyGuide 0.1 CONFIG REQUIRED)

add_executable(sturdy_guide_consumer main.cpp)
target_compile_features(sturdy_guide_consumer PRIVATE cxx_std_17)

# 导入的 target 已携带安装后的头文件路径与库位置。
target_link_libraries(sturdy_guide_consumer PRIVATE SturdyGuide::sturdy_guide)
