#ifndef AVX_IMPORTS_HPP
#define AVX_IMPORTS_HPP

#ifdef ENCLAVE
#include "simd-headers/emmintrin.h"
#include "simd-headers/smmintrin.h"
#include "simd-headers/tmmintrin.h"
#include "simd-headers/xmmintrin.h"
#include "simd-headers/avxintrin.h"
#include "simd-headers/avx2intrin.h"
#include "simd-headers/avx512fintrin.h"
#include "simd-headers/avx512cdintrin.h"
#include "simd-headers/avx512vlintrin.h"
#include "simd-headers/avx512bwintrin.h"
#include "simd-headers/avx512cdintrin.h"
#include "simd-headers/avx512fintrin.h"
#include "simd-headers/avx512vbmiintrin.h"
#include "simd-headers/avx512vbmi2intrin.h"
#else
#include <immintrin.h>
#endif

#endif //AVX_IMPORTS_HPP
