set(PBF_TEST_SOURCES test_pbf.cpp)

add_executable (pbf_test ${PBF_TEST_SOURCES})

target_link_libraries(pbf_test pbf_lib catch2)
