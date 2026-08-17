# 独立的可执行 demo：演示在项目里新增一个子目录，并由根文件 add_subdirectory 收录。
add_executable(qiaoruihao_demo main.cpp)

# 和仓库其它 target 保持一致，显式声明 C++17。
target_compile_features(qiaoruihao_demo PRIVATE cxx_std_17)
