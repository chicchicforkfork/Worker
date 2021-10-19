include(FetchContent)
FetchContent_Declare(gtest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)
FetchContent_MakeAvailable(gtest)

add_executable(test_EMPTY
    test_EMPTY.cpp)

target_include_directories(test_EMPTY
PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${EMPTY_SOURCE_DIR}/include
)

target_link_libraries(test_EMPTY
PRIVATE
    gtest_main
    pthread
)
