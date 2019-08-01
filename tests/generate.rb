#!/usr/bin/env ruby

TEST_REGEX = /(test_result ([^ ]+)\(.*\))/

outfile = File.open(ARGV.shift, "w")
inputs = ARGV

tests = inputs
  .map{|file| File.read(file)}
  .map{|data| data.scan(TEST_REGEX)}
  .flatten(1)

outfile.puts '#include "tests/tests.h"'
outfile.puts

outfile.puts tests
  .map{|test| test.first + ";"}
  .join("\n")

outfile.puts
outfile.puts "test_entry tests[] = {"

outfile.puts tests
  .map{|test| "    test(" + test.last[5..-1] + "),"}
  .join("\n")

outfile.puts "    {NULL, NULL},"

outfile.puts "};"
outfile.close
