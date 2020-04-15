#!/bin/bash
set -eio pipefail

build_dir=$(mktemp -d)
source_dir=${1:-.}

echo $build_dir
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S "$source_dir" -B "$build_dir"

cd "$build_dir"
echo "-- Running IWYU"
iwyu_tool.py -p . > iwyu.out

echo "-- Applying fixes"
fix_includes.py < iwyu.out

echo "-- Deleting build folder"
rm -rf $build_dir
