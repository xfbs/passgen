outfile = ARGV.shift
#outfile = File.open(outfile, "w")
outfile = STDOUT
inputs = ARGV

def strip_comments str
  str.gsub /\/\/[^\n]*\n/, "\n"
end

def strip_comment_blocks str
  str.gsub /\/\*.*?\*\//m, "\n"
end

def strip_preprocessor str
  str
end

module State
  INIT = 1
  ENUM = 2
  ENUM_NAME = 3
  ENUM_CONTENT = 4
  ENUM_CONTENT_WANT_EQ = 5
  ENUM_CONTENT_WANT_VALUE = 6
  ENUM_CONTENT_WANT_COMMA = 6
end

class Parser
end

def output_tokens output, enum_name, values
  output.puts "enum #{enum_name} {"
  values.each do |name, value|
    outputs.puts "  #{name} = #{value},"
  end
  outputs.puts "};"
end

def parse_tokens(output, tokens)
  state = State::INIT
  enum_name = ""
  current = ""
  value = 0
  values = {}

  tokens.each do |token|
    puts "token: #{token}"
    puts "state: #{state}"
    case state
    when State::INIT
      if token == 'enum'
        state = State::ENUM
      end
    when State::ENUM
      if token =~ /^[a-zA-Z_][a-zA-Z0-9_]*$/
        state = State::ENUM_NAME
        enum_name = token
      else
        state = State::INIT
      end
    when State::ENUM_NAME
      if token == '{'
        state = State::ENUM_CONTENT
      else
        state = State::INIT
      end
    when State::ENUM_CONTENT
      if token =~ /^}.*/
        output_values(output, enum_name, values)
        state = State::INIT
      elsif token =~ /^[a-zA-Z_][a-zA-Z0-9_]*,$/
        values[token[0..-2]] = value
        value += 1
      elsif token =~ /^[a-zA-Z_][a-zA-Z0-9_]*$/
        current = token
        state = State::ENUM_CONTENT_WANT_EQ
      else
        puts "error: unexpected token #{token}"
      end
    when State::ENUM_CONTENT_WANT_EQ
      if token == '}'
        values[current] = value
        value = 0
        output_values(output, enum_name, values)
        state = State::INIT
      elsif token == ','
        values[current] = value
        value += 1
        state = State::ENUM_CONTENT
      elsif token == '='
        state = State::ENUM_CONTENT_WANT_VALUE
      end
    when State::ENUM_CONTENT_WANT_VALUE
      case token
      when /^-?[0-9]+$/
        value = token.to_i
        values[current] = value
        value += 1
        state = State::ENUM_CONTENT_WANT_COMMA
      when /^-?[0-9]+,$/
        value = token[0..-2].to_i
        values[current] = value
        value += 1
        state = State::ENUM_CONTENT
      else
        puts "error: unexpected token #{token}"
      end
    when State::ENUM_CONTENT_WANT_COMMA
      case token
      when ","
        state = State::ENUM_CONTENT
      when /^}.*/
        output_values(output, enum_name, values)
        value = 0
        state = State::INIT
      else
        puts "error: unexpected token #{token}"
      end
    end
  end
end

inputs.each do |input|
  content = File.read(input)

  content = strip_comments(content)
  content = strip_comment_blocks(content)

  tokens = content.split /\s+/

  parse_tokens(outfile, tokens)
end



