#!/bin/sh
set -e

if [ "$#" -eq 0 ]; then
  FILES=$(find esp/src -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.ino' \))
else
  FILES="$@"
fi

echo "Running clang-format..."
clang-format --dry-run --Werror $FILES

echo "Running clang-tidy..."
for file in $FILES; do
  clang-tidy $file -- -Iesp/src
done