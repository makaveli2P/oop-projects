# frozen_string_literal: true

module SetCalc
  # The user typed a command that the calculator cannot accept. The message is
  # for the user.
  class CommandError < StandardError; end

  # One parsed command of the command loop. +name+ is one of NAMES. +argument+
  # is an array of numbers for X/Y/Z, a number for "a", the lambda source text
  # for "l", and nil for the others.
  Command = Data.define(:name, :argument, :source) do
    def to_s
      source
    end
  end

  # Parses one line of user input into a Command.
  module CommandParser
    SET_NAMES = %i[X Y Z].freeze

    USAGE = {
      X: "X values", Y: "Y values", Z: "Z values", a: "a i", r: "r", s: "s",
      u: "u", i: "i", c: "c", l: "l aString", q: "q"
    }.freeze

    INTEGER = /\A[+-]?\d+\z/
    DECIMAL = /\A[+-]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?\z/

    module_function

    def parse(line)
      text = line.to_s.scrub("?").strip
      raise CommandError, "type a command, for example: X 5,3,8" if text.empty?

      letter, rest = text.split(/\s+/, 2)
      name = command_name(letter)
      Command.new(name: name, argument: argument_for(name, rest.to_s.strip), source: text)
    end

    def command_name(letter)
      key = letter.length == 1 ? letter : nil
      key = key.upcase if key && %w[x y z].include?(key)
      key = key.downcase if key && !%w[X Y Z].include?(key)
      name = key&.to_sym
      return name if USAGE.key?(name)

      raise CommandError, "unknown command #{letter.inspect}. The commands are: #{USAGE.values.join(', ')}"
    end

    def argument_for(name, rest)
      case name
      when *SET_NAMES then parse_values(rest)
      when :a then parse_single(rest)
      when :l then parse_lambda_text(rest)
      else
        raise CommandError, "the command #{name} takes no argument" unless rest.empty?

        nil
      end
    end

    # "5,3,8", "5, 3, 8" and "5 3 8" all give [5, 3, 8]. An empty text gives
    # an empty set.
    def parse_values(text)
      text.split(/[\s,]+/).reject(&:empty?).map { |token| parse_number(token) }
    end

    def parse_single(text)
      values = parse_values(text)
      raise CommandError, "a needs one number, for example: a 7" unless values.size == 1

      values.first
    end

    def parse_lambda_text(text)
      raise CommandError, "l needs a lambda, for example: l ->(x) { x * 2 }" if text.empty?

      text
    end

    def parse_number(token)
      return Integer(token, 10) if INTEGER.match?(token)

      if DECIMAL.match?(token)
        value = quietly { Float(token, exception: false) } # 1e400 warns "out of range"
        return value if value&.finite?
      end
      raise CommandError, "#{token.inspect} is not a number"
    end

    def quietly
      verbose = $VERBOSE
      $VERBOSE = nil
      yield
    ensure
      $VERBOSE = verbose
    end
  end
end
