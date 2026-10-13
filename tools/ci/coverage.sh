#!/usr/bin/env bash
# Build with gcov instrumentation, run ctest, produce lcov report (src/ only) + HTML.
# 以 gcov 插樁建置、跑 ctest，產生 lcov 報告（僅 src/）與 HTML。
set -euo pipefail
BUILD=${BUILD:-build-cov}
OUT=${OUT:-coverage}
IGN=mismatch,negative,inconsistent,unused
cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug \
  -DSTONEPOTATOES_ENABLE_COVERAGE=ON -DSTONEPOTATOES_BUILD_EDITOR=OFF \
  -DSTONEPOTATOES_BUILD_EXAMPLES=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=ON
cmake --build "$BUILD" -j"$(nproc)"
ctest --test-dir "$BUILD" --output-on-failure --timeout 900
mkdir -p "$OUT"
lcov --capture --directory "$BUILD" --output-file "$OUT/all.info" --ignore-errors "$IGN"
lcov --extract "$OUT/all.info" "$PWD/src/*" --output-file "$OUT/lcov.info" --ignore-errors "$IGN"
lcov --summary "$OUT/lcov.info" --ignore-errors "$IGN" | tee "$OUT/summary.txt"
genhtml "$OUT/lcov.info" --output-directory "$OUT/html" --ignore-errors "$IGN" >/dev/null
