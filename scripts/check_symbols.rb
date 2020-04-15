#!/usr/bin/env ruby
require 'optparse'

options = {}
options[:prefix] = []
options[:regex] = []

parser = OptionParser.new do |opts|
  opts.banner = "Usage: check_symbols.rb [options]"

  opts.on("-nPATH", "--nm=PATH", "Path to nm executable") do |nm|
    options[:nm] = nm
  end

  opts.on("-pPREFIX", "--prefix=PREFIX", "Prefix to allow") do |prefix|
    options[:prefix] << prefix
  end

  opts.on("-rREGEX", "--regex=REGEX", "Regex to allow") do |regex|
    options[:regex] << Regexp.new(regex)
  end

  opts.on("-h", "--help", "Show this help text") do
    puts opts
  end
end

parser.parse!

options[:nm] ||= 'nm'
options[:okay] = true

if ARGV.empty?
  puts parser
  exit 1
end

def check_symbol options, file, symbol
  # on macOS, symbols are prefixed with an underscore. strip these out.
  if /darwin/ =~ RUBY_PLATFORM
    symbol = symbol.gsub /^_/, ""
  end

  match = false

  options[:prefix].each do |prefix|
    if symbol.start_with? prefix
      match = true
    end
  end

  options[:regex].each do |regex|
    if regex.match? symbol
      match = true
    end
  end

  unless match
    puts "error: symbol #{symbol} in #{file} doesn't match!"
    options[:okay] = false
  end
end

ARGV.each do |file|
  symbols = `#{options[:nm]} -g #{file.inspect}`

  unless $?.success?
    puts "error running #{options[:nm]} on file #{file.inspect}, exiting!"
    exit 1
  end

  current_file = ""

  symbols.split("\n").each do |symbol|
    case symbol
    when /^(.+):$/
      current_file = $1
    when /^\s*$/
      # ignore
    when /^([0-9a-f]+|\s+) ([a-zA-Z?]) ([a-zA-Z_][a-zA-Z0-9_]*)$/
      if $2 == 'T'
        check_symbol options, current_file, $3
      end
    else
      puts "error: can't parse '#{symbol.inspect}'"
    end
  end
end

unless options[:okay]
  exit 1
end
