#!/usr/bin/env ruby

TEST_REGEX = /(test_result test_([^ ]+)\(.*\))/

outfile = File.open(ARGV.shift, "w")
inputs = ARGV

tests = inputs
  .map{|file| File.read(file).force_encoding('UTF-8')}
  .map{|data| data.scan(TEST_REGEX)}
  .flatten(1)

outfile.puts '#include "tests.h"'
outfile.puts

outfile.puts tests
  .map{|test| test.first + ";"}
  .join("\n")

outfile.puts
outfile.puts "test_entry tests[] = {"

outfile.puts tests
  .map{|test| "    test(" + test.last + "),"}
  .join("\n")

outfile.puts "    {NULL, NULL},"

outfile.puts "};"

outfile.puts
outfile.puts "size_t tests_len = #{tests.size};"
outfile.close
