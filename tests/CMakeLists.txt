function(add_sturdy_guide_test target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE SturdyGuide::sturdy_guide) 
  add_test(NAME ${target} COMMAND ${target})
endfunction()

add_sturdy_guide_test(greeting_test greeting_test.cpp)
add_sturdy_guide_test(text_test text_test.cpp)
add_sturdy_guide_test(greeting_plusplus_test greeting_plus.cpp)
