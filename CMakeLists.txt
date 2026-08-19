cmake_minimum_required(VERSION 3.16)
project(SturdyGuide)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== 自动拉取GTest 【核心新增】 =====
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
  GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# =====================================
include(CTest)
# 编译业务库 src 子目录
add_subdirectory(src)
# 编译单元测试 tests 子目录
add_subdirectory(tests)
