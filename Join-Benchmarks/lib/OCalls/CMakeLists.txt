add_library(ocalls-u STATIC src/ocalls.cpp src/cpu_mapping.cpp)
target_include_directories(ocalls-u PRIVATE ${APP_INC_DIRS})
target_include_directories(ocalls-u PUBLIC include include_t)
set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${APP_CXX_FLAGS})
target_link_libraries(ocalls-u perf_event logger shared_headers)
target_compile_features(ocalls-u PUBLIC cxx_std_17)

add_library(ocalls-t INTERFACE)
target_include_directories(ocalls-t INTERFACE include_t)
