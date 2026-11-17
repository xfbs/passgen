#!/bin/sh
# Prints the version to use for packages of nightly builds, such as
# `0.2.0+nightly20261117`: the version that was released last, and the date of
# the build. This sorts after the release, and before the next one.
#
# In GitLab CI, the date is that of the pipeline, so that all jobs of a
# pipeline agree on it. Otherwise, it is today's date.
set -eu

source_dir="$(cd "$(dirname "$0")/.." && pwd)"
version="$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' "$source_dir/CMakeLists.txt" | head -n 1)"
if test -z "$version"; then
    echo "cannot find the version in CMakeLists.txt" >&2
    exit 1
fi

if test -n "${CI_PIPELINE_CREATED_AT:-}"; then
    # this is a timestamp such as 2026-11-17T12:34:56Z
    date="$(echo "$CI_PIPELINE_CREATED_AT" | cut -c 1-10 | tr -d -)"
else
    date="$(date -u +%Y%m%d)"
fi

echo "$version+nightly$date"
