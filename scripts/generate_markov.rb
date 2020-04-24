#!/usr/bin/env ruby
require 'zlib'

if ARGV.empty?
  puts "generate_markov.rb <dictionary_file> <name>"
  puts
  puts "Generates a markov chain from the given dictionary file (which is assumed"
  puts "to be a GZipped text file, with one word per line. Outputs the resulting"
  puts "markov chain C code on standard output."
  puts
  puts "Example:"
  puts
  puts "  ./scripts/generate_markov.rb ./data/english.txt.gz passgen_markov_english"
  exit
end

dictionary_file = Zlib::GzipReader.open(ARGV[0]).read.force_encoding('UTF-8')
name = ARGV[1]

words = dictionary_file.split("\n")

table = Hash.new{|h, v| h[v] = Hash.new{|h, v| h[v] = Hash.new{|h, v| h[v] = 0}}}

n = 2

words.each do |word|
    letters = ["\0"] * n + word.chars + ["\0"]

    (0...(letters.size - n)).each do |i|
        a, b, c = letters[i..(i+2)]
        table[a][b][c] += 1
    end
end

puts '#include "passgen/pronounceable_private.h"'
puts
puts "const struct markov #{name} = {"
puts '    .list = (const struct markov2[]) {'

table.sort_by{|k, v| k}.each do |first, tsecond|
puts '        {'
puts "            .codepoint = 0x#{first.ord.to_s(16)},"
puts "            .list = (const struct markov1[]){"

tsecond.sort_by{|k, v| k}.each do |second, tthird|

puts '                {'
puts "                    .codepoint = 0x#{second.ord.to_s(16)},"
puts "                    .list = (const struct markov0[]){"

tthird.sort_by{|k, v| -v}.each do |third, count|

puts '                        {'
puts "                            .codepoint = 0x#{third.ord.to_s(16)},"
puts "                            .frequency = #{count},"
puts '                        },'

end

puts '                    },'
puts "                    .list_len = #{tthird.size},"
puts "                    .frequency_sum = #{tthird.values.sum},"
puts '                },'

end

puts '            },'
puts "            .list_len = #{tsecond.size},"
puts '        },'
end

puts '    },'
puts "    .list_len = #{table.size},"
puts '};'
