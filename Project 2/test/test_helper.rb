# frozen_string_literal: true

$LOAD_PATH.unshift File.expand_path("../lib", __dir__)

require "minitest/autorun"
require "stringio"
require "tmpdir"
require "ali"
require "ali/plain_mode"
require "ali/tui"
require "ali/cli"

module ALITestHelpers
  ROOT = File.expand_path("..", __dir__)

  def root_path(*parts) = File.join(ROOT, *parts)

  def assemble(source)
    result = ALI::Assembler.assemble(source)

    assert_predicate result, :ok?, -> { result.errors.join("\n") }
    result.program
  end

  def machine(source) = ALI::Machine.new(assemble(source))

  def errors(source)
    result = ALI::Assembler.assemble(source)

    refute_predicate result, :ok?
    result.errors
  end

  def op_classes(step) = step.micro_ops.map { it.class.name.split("::").last }
end

Minitest::Test.include(ALITestHelpers)
