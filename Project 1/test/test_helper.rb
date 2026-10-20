# frozen_string_literal: true

require "minitest/autorun"
require "stringio"
require "set_calc"
require "set_calc/cli"
require "set_calc/tui/render"

module TestHelpers
  def tree(*values)
    SetCalc::BST.from(values)
  end

  # A calculator after each line of +lines+.
  def calculator_after(*lines)
    calc = SetCalc::Calculator.new
    lines.each do |line|
      result = calc.run(line)
      raise "#{line}: #{result.summary}" if result.error
    end
    calc
  end

  def frames_for(calc, line)
    tracer = SetCalc::Tracer.new(calc, command: line)
    result = calc.run(line, tracer: tracer)
    [result, tracer.frames]
  end
end

Minitest::Test.include(TestHelpers)
