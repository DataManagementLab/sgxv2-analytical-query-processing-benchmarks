set(UTILITY_SRCS
        src/genzipf.cpp
        src/commons.cpp
        src/ErrorSupport.cpp
        src/generator.cpp
        src/parallel_sort.cpp
        src/utility.cpp
)

add_library(app_utilities STATIC ${UTILITY_SRCS})
target_include_directories(app_utilities PUBLIC include)
target_link_libraries(app_utilities logger shared_headers perf_event)
target_compile_features(app_utilities PUBLIC cxx_std_17)
