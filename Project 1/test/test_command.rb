# frozen_string_literal: true

require "test_helper"

class TestCommand < Minitest::Test
  def parse(line)
    SetCalc::CommandParser.parse(line)
  end

  def test_set_values
    command = parse("X 5,3,8")

    assert_equal :X, command.name
    assert_equal [5, 3, 8], command.argument
  end

  def test_value_separators_and_signs
    assert_equal [5, -3, 8, 2.5], parse("Y 5, -3 8,2.5").argument
    assert_equal [1000.0, 0.5], parse("Z +1e3,.5").argument
  end

  def test_empty_value_list_gives_empty_set
    assert_equal [], parse("X").argument
  end

  def test_lowercase_set_name
    assert_equal :Y, parse("y 1").name
  end

  def test_insert
    assert_equal 7, parse("a 7").argument
    assert_raises(SetCalc::CommandError) { parse("a") }
    assert_raises(SetCalc::CommandError) { parse("a 1,2") }
  end

  def test_commands_without_argument
    %w[r s u i c q].each { |letter| assert_equal letter.to_sym, parse(letter).name }
    assert_raises(SetCalc::CommandError) { parse("u 5") }
  end

  def test_lambda_keeps_the_text
    assert_equal "->(x) { x * 2 }", parse("l ->(x) { x * 2 }").argument
    assert_raises(SetCalc::CommandError) { parse("l") }
  end

  def test_bad_numbers
    %w[abc 0x1A 1e400 nan 1_000 5..7].each do |token|
      assert_raises(SetCalc::CommandError, token) { parse("X #{token}") }
    end
  end

  def test_unknown_command
    error = assert_raises(SetCalc::CommandError) { parse("foo") }

    assert_match(/unknown command "foo"/, error.message)
    assert_raises(SetCalc::CommandError) { parse("   ") }
  end

  def test_invalid_utf8_is_a_command_error
    error = assert_raises(SetCalc::CommandError) { parse((+"X 1\xff").force_encoding(Encoding::UTF_8)) }

    assert_match(/"1\?" is not a number/, error.message)
  end
end
