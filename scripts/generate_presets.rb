#!/usr/bin/env ruby
require 'toml'

outfile = ARGV.shift
infile  = ARGV.shift
varname = ARGV.shift || "pattern_presets"

outfile = File.open(outfile, 'w')
infile  = File.read(infile)

content = TOML.load(infile)

outfile.puts '#include "passgen/passgen2.h"'
outfile.puts "const pattern_preset #{varname}[] = {"

content.each do |name, data|
  outfile.puts "  {.name = #{name.inspect}, .format = #{data["pattern"].inspect}, .description = #{data["description"].inspect}},"
end

outfile.puts '};'

outfile.puts "const size_t #{varname}_length = #{content.length};"
