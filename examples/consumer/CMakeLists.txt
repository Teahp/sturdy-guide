cmake_minimum_required(VERSION 3.16)

project(SturdyGuideConsumer LANGUAGES CXX)

find_package(SturdyGuide 0.1 CONFIG REQUIRED)

add_executable(sturdy_guide_consumer main.cpp)
target_compile_features(sturdy_guide_consumer PRIVATE cxx_std_17)
target_link_libraries(sturdy_guide_consumer PRIVATE SturdyGuide::sturdy_guide)

