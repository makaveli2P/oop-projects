# frozen_string_literal: true

require "optparse"
require_relative "classic"

module SetCalc
  # The command-line interface of main.rb.
  module CLI
    USAGE = <<~TEXT
      Usage: ruby main.rb [--plain]
             ruby main.rb --render [--cmd COMMAND]... [--step N] [--keys KEYS] [--size WxH] [--color]

      Without options, main.rb starts the terminal visualizer. When the input or
      the output is not a terminal, it starts the classic mode.
    TEXT

    module_function

    def run(argv, input: $stdin, output: $stdout, error: $stderr)
      options = parse(argv)
      return show_help(output) if options[:help]
      return render(options, output) if options[:render]
      return Classic.new(input: input, output: output).run if options[:plain] || !(input.tty? && output.tty?)

      require_relative "tui/app"
      TUI::App.new(input: input, output: output).run
    rescue OptionParser::ParseError, ArgumentError => e
      error.puts "main.rb: #{e.message}"
      error.puts USAGE
      2
    end

    def parse(argv)
      options = { commands: [], step: nil, keys: "", width: 120, height: 36, color: false }
      parser(options).parse!(argv.dup).then do |rest|
        raise OptionParser::InvalidArgument, rest.join(" ") unless rest.empty?
      end
      options
    end

    def parser(options)
      OptionParser.new do |o|
        o.on("--plain") { options[:plain] = true }
        o.on("--render") { options[:render] = true }
        o.on("--cmd COMMAND") { |c| options[:commands] << c }
        o.on("--step N", Integer) { |n| options[:step] = n }
        o.on("--keys KEYS") { |k| options[:keys] = k }
        o.on("--size WxH", /\A\d+x\d+\z/) { |s| options[:width], options[:height] = s.split("x").map(&:to_i) }
        o.on("--color") { options[:color] = true }
        o.on("-h", "--help") { options[:help] = true }
      end
    end

    def show_help(output)
      output.puts USAGE
      0
    end

    def render(options, output)
      require_relative "tui/render"
      output.puts TUI::Render.frame(commands: options[:commands], step: options[:step], keys: options[:keys],
                                    width: options[:width], height: options[:height], color: options[:color])
      0
    end
  end
end
