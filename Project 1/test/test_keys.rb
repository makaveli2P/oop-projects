# frozen_string_literal: true

require "test_helper"

class TestKeys < Minitest::Test
  Keys = SetCalc::TUI::Keys

  def test_printable_and_control
    assert_equal ["a", " ", "7", :enter, :tab, :backspace, :ctrl_c], Keys.decode("a 7\r\t\x7F\x03")
  end

  def test_escape_sequences
    assert_equal %i[up down right left end home page_up], Keys.decode("\e[A\e[B\e[C\e[D\e[F\eOH\e[5~")
  end

  def test_lone_escape_and_unknown_sequence
    assert_equal [:escape], Keys.decode("\e")
    assert_equal ["x"], Keys.decode("\e[99Qx")
  end

  def test_invalid_utf8_is_dropped
    assert_equal ["a"], Keys.decode("a\xFF".b)
  end

  def test_script
    assert_equal ["a", " ", "7", :enter, :right, " "], Keys.script("a 7<enter><right><space>")
    assert_raises(ArgumentError) { Keys.script("<nope>") }
  end
end
