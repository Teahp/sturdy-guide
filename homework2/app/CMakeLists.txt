# 命令行程序是独立 target，只通过公共 API 使用 sturdy_guide 库。
add_executable(sturdy_guide_cli main.cpp)

# 保持可执行文件位于顶层构建目录，方便从 README 直接运行。
set_target_properties(
  sturdy_guide_cli
  PROPERTIES
    OUTPUT_NAME sturdy-guide
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
target_link_libraries(sturdy_guide_cli PRIVATE SturdyGuide::sturdy_guide)
