# frozen_string_literal: true

require "optparse"
require_relative "../ali"
require_relative "plain_mode"
require_relative "tui"

module ALI
  # The command line: ali [--plain] [FILE], or ali --render FILE [options].
  class CLI
    USAGE = <<~TEXT
      Usage: ali [FILE]                 the terminal visualizer
             ali --plain [FILE]         the classic text mode (s, a, q)
             ali --render FILE [--steps N] [--ops N] [--keys KEYS] [--size WxH] [--color]
    TEXT

    def self.start(argv = ARGV, input: $stdin, output: $stdout, error: $stderr)
      new(input, output, error).run(argv)
    end

    def initialize(input, output, error)
      @in = input
      @out = output
      @err = error
    end

    # Returns the exit status.
    def run(argv)
      options = parse(argv.dup)
      return options if options.is_a?(Integer)

      case options[:mode]
      when :help then help
      when :version then version
      when :plain then PlainMode.new(input: @in, output: @out, error: @err).run(options[:file])
      when :render then render(options)
      else TUI::App.run(options[:file] && (SourceFile.resolve(options[:file]) || options[:file]),
                        input: @in, output: @out, error: @err)
      end
    end

    private

    def parse(argv)
      options = { mode: :tui, steps: 0, ops: 0, keys: "", size: TUI::Render::DEFAULT_SIZE, color: false }
      files = parser(options).parse(argv)
      return usage_error("too many arguments") if files.size > 1

      options[:file] = files.first
      return usage_error("--render needs a file") if options[:mode] == :render && options[:file].nil?

      options
    rescue OptionParser::ParseError, ArgumentError => e
      usage_error(e.message)
    end

    def parser(options)
      OptionParser.new do |o|
        o.banner = USAGE
        o.on("--plain") { options[:mode] = :plain }
        o.on("--render") { options[:mode] = :render }
        o.on("--steps N", Integer) { options[:steps] = non_negative(it, "--steps") }
        o.on("--ops N", Integer) { options[:ops] = non_negative(it, "--ops") }
        o.on("--keys KEYS") { options[:keys] = it }
        o.on("--size WxH") { options[:size] = TUI::Render.parse_size(it) }
        o.on("--color") { options[:color] = true }
        o.on("-h", "--help") { options[:mode] = :help }
        o.on("-v", "--version") { options[:mode] = :version }
      end
    end

    def non_negative(value, name)
      raise ArgumentError, "#{name} must be 0 or more" if value.negative?

      value
    end

    def help
      @out.puts USAGE
      0
    end

    def version
      @out.puts "ali #{VERSION}"
      0
    end

    def usage_error(message)
      @err.puts "ali: #{message}"
      @err.puts USAGE
      2
    end

    def render(options)
      path = SourceFile.resolve(options[:file]) || options[:file]
      frame, status = TUI::Render.frame(path, **options.slice(:steps, :ops, :keys, :size, :color))
      @out.puts frame
      status
    end
  end
end
