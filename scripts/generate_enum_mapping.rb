#!/usr/bin/env ruby
# Crude parser for C headers, parses enum definitions and generates a mapping
# from them (sorted by name and by value) that can be used to convert between
# enums and strings easily.

def strip_comments str
  str.gsub /\/\/[^\n]*\n/, "\n"
end

def strip_comment_blocks str
  str.gsub /\/\*.*?\*\//m, "\n"
end

def strip_preprocessor str
  str.gsub /^#([^\n]+\\\n)+[^\n]*\n/m, "\n"
end

class Parser
  module State
    INIT = 1
    ENUM = 2
    ENUM_NAME = 3
    ENUM_CONTENT = 4
    ENUM_CONTENT_WANT_EQ = 5
    ENUM_CONTENT_WANT_VALUE = 6
    ENUM_CONTENT_WANT_COMMA = 7
    DONE = 8
  end

  attr_accessor :enum_name, :values

  def initialize
    @state = State::INIT
    @enum_name = ""
    @current = ""
    @value = 0
    @values = {}
  end

  def add_value(name: @current, value: @value)
    @value = value
    @values[name] = @value
    @value += 1
  end

  def error token
    STDERR.puts "error: unexpected token #{token}"
    @state = State::INIT
  end

  def parse_init token
    if token == 'enum'
      @state = State::ENUM
    end
  end

  def parse_enum token
    case token
    when /^[a-zA-Z_][a-zA-Z0-9_]*$/
      @state = State::ENUM_NAME
      @enum_name = token
    else
      error token
    end
  end

  def parse_enum_name token
    case token
    when '{'
      @state = State::ENUM_CONTENT
    when /^[a-zA-Z_][a-zA-Z0-9_]*;?$/
      # variable declaration
      @state = State::INIT
    else
      error token
    end
  end

  def parse_enum_content token
    case token
    when /^}/
      @state = State::DONE
    when /^[a-zA-Z_][a-zA-Z0-9_]*,$/
      add_value(name: token[0..-2])
    when /^[a-zA-Z_][a-zA-Z0-9_]*$/
      @current = token
      @state = State::ENUM_CONTENT_WANT_EQ
    else
      error token
    end
  end

  def parse_enum_content_want_eq token
    case token
    when /^}/
      add_value
      @state = State::DONE
    when ','
      add_value()
      @state = State::ENUM_CONTENT
    when '='
      @state = State::ENUM_CONTENT_WANT_VALUE
    else
      error token
    end
  end

  def parse_enum_content_want_value token
    case token
    when /^-?[0-9]+$/
      add_value(value: token.to_i)
      @state = State::ENUM_CONTENT_WANT_COMMA
    when /^-?[0-9]+,$/
      add_value(value: token[0..-2].to_i)
      @state = State::ENUM_CONTENT
    else
      error token
    end
  end

  def parse_enum_content_want_comma token
    case token
    when ","
      @state = State::ENUM_CONTENT
    when /^}/
      @state = State::DONE
    else
      error token
    end
  end

  def parse token
    case @state
    when State::INIT
      parse_init token
    when State::ENUM
      parse_enum token
    when State::ENUM_NAME
      parse_enum_name token
    when State::ENUM_CONTENT
      parse_enum_content token
    when State::ENUM_CONTENT_WANT_EQ
      parse_enum_content_want_eq token
    when State::ENUM_CONTENT_WANT_VALUE
      parse_enum_content_want_value token
    when State::ENUM_CONTENT_WANT_COMMA
      parse_enum_content_want_comma token
    when State::DONE
      throw 'done parsing'
    else
      throw 'unknown state'
    end
  end

  def done?
    return @state == State::DONE
  end

  def check
    reverse = {}
    @values.each do |name, value|
      if reverse[value]
        throw "#{name} and #{reverse[value]} alias"
      end
    end
  end

  def puts output
    output.puts '#include "passgen/enum_mapping.h"'
    output.puts "size_t #{enum_name}_enum_length = #{values.size};"

    output.puts "const struct passgen_enum_mapping #{enum_name}_enum_by_name[] = {"
    @values.sort_by{|name, _| name}.each do |name, value|
      output.puts "  {.name = \"#{name}\", .value = #{value}},"
    end
    output.puts "};"

    output.puts "const struct passgen_enum_mapping #{enum_name}_enum_by_value[] = {"
    @values.sort_by{|_, value| value}.each do |name, value|
      output.puts "  {.name = \"#{name}\", .value = #{value}},"
    end
    output.puts "};"
  end
end

def parse_tokens(output, tokens)
  parser = Parser.new

  tokens.each do |token|
    parser.parse token

    if parser.done?
      parser.check
      parser.puts(output)
      parser = Parser.new
    end
  end
end

def parse_files output, inputs
  inputs.each do |input|
    content = File.read input

    content = strip_comments content
    content = strip_comment_blocks content
    content = strip_preprocessor content

    tokens = content.split /\s+/

    parse_tokens output, tokens
  end
end

outfile = ARGV.shift
outfile = File.open(outfile, "w")
inputs = ARGV

parse_files outfile, inputs
