# gflags for parsing commandline flags
FetchContent_Declare(
  gflags
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG v2.2.2)
FetchContent_MakeAvailable(gflags)

# include subprojects / targets
add_subdirectory(SimdScanMulti)
add_subdirectory(RadixPartitioning)
add_subdirectory(WriteBench)
