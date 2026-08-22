include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# ============================================
# 2. 辅助函数：复用“测试程序 + 库依赖 + CTest 注册”
# ============================================
function(add_sturdy_guide_test target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE SturdyGuide::sturdy_guide)
  add_test(NAME ${target} COMMAND ${target})
endfunction()

# ============================================
# 3. 注册测试程序（已有 + 新增）
# ============================================
add_sturdy_guide_test(greeting_test greeting_test.cpp)
add_sturdy_guide_test(text_test text_test.cpp)

# ---------- 新增 hello_test ----------
add_sturdy_guide_test(hello_test hello_test.cpp)
# 因为 hello_test 使用了 gtest，需要额外链接 gtest 库
target_link_libraries(hello_test PRIVATE gtest_main gtest)
