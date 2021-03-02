//
// Created by Matt Woicik on 3/1/21.
//

// The code in this file is taken from:
// https://github.com/geofflangdale/simdcsv
#ifdef __AVX2__
#ifndef NORMAL_SIMDPARSERHELPERS_H
#define NORMAL_SIMDPARSERHELPERS_H
#include <stdint.h>
#include <immintrin.h>

typedef struct ParsedCSV {
  uint32_t n_indexes{0};
  uint32_t *indexes;
} ParsedCSV;
typedef struct simd_input {
  __m256i lo;
  __m256i hi;
} simd_input;

bool find_indexes(const uint8_t * buf, size_t len, struct ParsedCSV & pcsv);
#endif //NORMAL_SIMDPARSERHELPERS_H
#endif