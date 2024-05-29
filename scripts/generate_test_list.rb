#!/usr/bin/env ruby

TEST_REGEX = /(test_result test_([^ ]+)\(.*\))/
IFDEF_REGEX = /(#ifdef|#ifndef|#else|#endif)/

outfile = File.open(ARGV.shift, "w")
inputs = ARGV

outfile.puts '#include "tests.h"'
outfile.puts '#include "passgen/config.h"'
outfile.puts

inputs.each do |input|
  File.read(input).force_encoding('UTF-8').lines.each do |line|
    line.scan(TEST_REGEX).each do |test|
      outfile.puts test.first + ';'
    end
  end
end

outfile.puts
outfile.puts "test_entry tests[] = {"

inputs.each do |input|
  File.read(input).force_encoding('UTF-8').lines.each do |line|
    if IFDEF_REGEX =~ line
      outfile.puts line
    end

    line.scan(TEST_REGEX).each do |test|
      outfile.puts "    test(" + test.last + "),"
    end
  end
end

outfile.puts "    {NULL, NULL},"

outfile.puts "};"

outfile.puts
outfile.puts "size_t tests_len = sizeof(tests) / sizeof(tests[0]) - 1;" #{tests.size};"
outfile.close
