#!/usr/bin/env ruby

# use a dictionary file like https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt

dictionary_file = File.read(ARGV[0])

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
puts 'const struct markov english = {'
puts '    .list = (const struct markov2[]) {'

table.sort_by{|k, v| k}.each do |first, tsecond|
puts '        {'
puts "            .codepoint = 0x#{first.bytes[0].to_s(16)},"
puts "            .list = (const struct markov1[]){"

tsecond.sort_by{|k, v| k}.each do |second, tthird|

puts '                {'
puts "                    .codepoint = 0x#{second.bytes[0].to_s(16)},"
puts "                    .list = (const struct markov0[]){"

tthird.sort_by{|k, v| k}.each do |third, count|

puts '                        {'
puts "                            .codepoint = 0x#{third.bytes[0].to_s(16)},"
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
