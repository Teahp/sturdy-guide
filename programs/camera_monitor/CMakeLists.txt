# Camera homework final layout: core library, app, and hardware-free tests.
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

option(STURDY_GUIDE_BUILD_CAMERA_STARTER
       "Build the original single-threaded camera starter" OFF)

if(STURDY_GUIDE_BUILD_CAMERA_STARTER)
  add_subdirectory(starter)
endif()

add_subdirectory(src)
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
