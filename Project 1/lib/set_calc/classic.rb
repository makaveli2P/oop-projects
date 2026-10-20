# frozen_string_literal: true

require_relative "calculator"

module SetCalc
  # The classic mode: the command loop of the assignment. It reads one command
  # for each line, executes it, and prints the three sets.
  class Classic
    BANNER = "Set calculator. Commands: X values, Y values, Z values, a i, r, s, u, i, c, l aString, q."

    def initialize(input: $stdin, output: $stdout, prompt: nil)
      @input = input
      @output = output
      @prompt = prompt.nil? ? input.tty? : prompt
      @calculator = Calculator.new
    end

    def run
      @output.puts BANNER if @prompt
      while (line = read_line)
        next if line.strip.empty?

        result = @calculator.run(line)
        break if result.quit

        report(result)
      end
      0
    end

    private

    def read_line
      @output.print "> " if @prompt
      @output.flush
      # A byte that is not valid in the encoding becomes "?", so the command
      # gives an "error:" line and does not stop the loop.
      @input.gets&.scrub("?")
    end

    def report(result)
      if result.error
        @output.puts "error: #{result.summary}"
        return
      end
      result.output.each { |line| @output.puts line }
      Calculator::SET_NAMES.each { |name| @output.puts "#{name} = #{@calculator[name]}" }
    end
  end
end
