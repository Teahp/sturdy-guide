find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test_session test_session.cpp)
target_link_libraries(test_session
    camera_lib
    ${GTEST_LIBRARIES}
    ${OpenCV_LIBS}
    Threads::Threads
)
add_test(NAME test_session COMMAND test_session)
