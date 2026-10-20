# frozen_string_literal: true

require "test_helper"

class TestTUITerminal < Minitest::Test
  def decode(bytes) = ALI::TUI::Terminal.new(StringIO.new, StringIO.new).send(:decode, bytes)

  def test_plain_keys_and_named_keys
    assert_equal [" ", "s", "R", "Enter", "Enter", "q"], decode(" sR\r\n\u0003")
    assert_equal %w[Up Down Up Escape], decode("\e[A\e[B\eOA\e")
  end

  def test_unknown_escape_sequences_are_dropped
    assert_equal ["x"], decode("\e[1;5Cx")
  end

  def test_mouse_clicks
    click = ALI::TUI::Terminal::Click.new(x: 4, y: 9)

    assert_equal [click], decode("\e[<0;5;10M\e[<0;5;10m")
    assert_empty decode("\e[<2;5;10M")
  end

  def test_canvas_clips_and_keeps_width
    canvas = ALI::TUI::Canvas.new(5, 2)
    canvas.text(3, 0, "abcdef").text(-1, 1, "xyz").text(0, 5, "nothing")

    assert_equal ["   ab", "yz   "], canvas.lines
  end
end
