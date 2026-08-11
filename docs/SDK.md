# SDK、库与工具链

## SDK 是什么

SDK（Software Development Kit）是帮助开发者面向某个平台或产品开发的一组材料。一个 SDK 通常包含：

- 供程序调用的库；
- 描述公共 API 的头文件；
- CMake 包配置或其他构建集成文件；
- 文档、示例程序和测试工具；
- 有时还会包含专用编译器、调试器或模拟器。

SDK 不等同于单个库，也不等同于编译器。G++、Clang++、CMake 和 Ninja 是构建工具；本项目安装后的库、头文件、CMake 包配置、文档与示例共同组成一个小型 SDK。

## 本项目中的对应关系

| 内容 | 位置 | 作用 |
| --- | --- | --- |
| 公共 API | `include/sturdy_guide/` | SDK 使用者可以包含的头文件 |
| 库实现 | `src/` | 被编译进 `libsturdy_guide.a` |
| CMake 集成 | `cmake/` 和安装规则 | 支持使用 `find_package(SturdyGuide)` |
| 使用示例 | `examples/consumer/` | 演示另一个项目如何使用安装后的 SDK |
| 自动测试 | `tests/` | 验证公共 API 的行为 |

## 安装并从外部项目使用

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
cmake --install build --prefix install

cmake -S examples/consumer -B consumer-build -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build consumer-build
./consumer-build/sturdy_guide_consumer
```

外部项目只依赖公共头文件和导出的 CMake target：

```cmake
find_package(SturdyGuide CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE SturdyGuide::sturdy_guide)
```

