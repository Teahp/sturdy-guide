find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

add_subdirectory(src)

add_subdirectory(app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
