# frozen_string_literal: true

require "test_helper"

class TestScreen < Minitest::Test
  SIZES = [[80, 22], [100, 30], [120, 36], [160, 48]].freeze

  def render(commands, step: nil, keys: "", width: 120, height: 36)
    SetCalc::TUI::Render.frame(commands: commands, step: step, keys: keys, width: width, height: height)
  end

  def assert_fits(text, width, height)
    lines = text.split("\n", -1)

    assert_operator lines.size, :<=, height
    lines.each { |line| assert_operator line.length, :<=, width, line }
  end

  def scenarios
    [
      [[]],
      [["X 5,3,8,1,4,7,9", "Y 1,9,10,2", "a 6"], 3],
      [["X #{(1..40).to_a.join(',')}", "a 41"], 30],
      [["X 5,3,8,1,4,7,9", "Y 4,1,9,12", "i"], 12],
      [["X 50,25,75,12,37,62,87", "c"], 5],
      [["X 1", "Y 2", "Z 3", "r"], 1],
      [["X 1.5,-2,1000000", "l ->(x) { x.to_s * 3 }"]],
      [["X 1,abc"]]
    ]
  end

  def test_every_frame_fits_at_every_size
    SIZES.each do |width, height|
      scenarios.each do |commands, step|
        assert_fits(render(commands, step: step, width: width, height: height), width, height)
      end
    end
  end

  def test_all_steps_of_a_union_fit
    calc_frames = SetCalc::TUI::Player.new.submit("X 5,3,8").submit("Y 1,4,9,12").submit("u").frames.size

    (1..calc_frames).each do |step|
      assert_fits(render(["X 5,3,8", "Y 1,4,9,12", "u"], step: step, width: 80, height: 22), 80, 22)
    end
  end

  def test_header_narration_and_position
    text = render(["X 5,3,8", "a 7"], step: 2)

    assert_match(/Set calculator/, text.lines.first)
    assert_match(/Paused/, text.lines.first)
    assert_includes text, "▸ 7 > 5, go right."
    assert_includes text, "a 7 · step 2 of 5"
  end

  def test_listing_and_stats
    text = render(["X 5,3,8"])

    assert_includes text, "{3, 5, 8}"
    assert_includes text, "3 elements · height 2"
  end

  def test_long_listing_is_cut
    text = render(["X #{(1..300).to_a.join(',')}"], width: 80, height: 22)

    assert_match(/… \d+ more\}/, text)
  end

  def test_error_message
    assert_includes render(["X 1,abc"]), "error: \"abc\" is not a number"
  end

  def test_lambda_results_message
    assert_includes render(["X 1,2", "l ->(x) { x * 2 }"]), "[2, 4]"
  end

  def test_wide_and_control_characters_are_escaped
    text = render(["X 1", "l ->(x) { \"\\e[2J日\" }"])

    refute_includes text, "\e"
    assert_includes text, "\\u{65E5}"
  end

  def test_typed_input_and_placeholder
    assert_includes render([]), "› type a command"
    assert_includes render([], keys: "a 12"), "› a 12"
  end

  def test_too_small
    text = render([], width: 60, height: 15)

    assert_includes text, "The terminal is too small."
    assert_fits(text, 60, 15)
  end

  def test_color_output_has_ansi_codes
    colored = SetCalc::TUI::Render.frame(commands: ["X 5,3", "a 4"], step: 2, color: true)

    assert_includes colored, "\e[1;36m"
  end

  def test_cursor_is_on_the_input_line
    player = SetCalc::TUI::Player.new
    "a 7".each_char { |char| player.press(char) }

    assert_equal [7, 34], SetCalc::TUI::Screen.new(player, 120, 36).cursor
  end

  def test_scroll_moves_at_once_after_overscroll
    commands = ["X #{(1..20).to_a.join(',')}"]
    deep = render(commands, keys: "<down>" * 30, width: 80, height: 22)
    back = render(commands, keys: "#{'<down>' * 30}<up>", width: 80, height: 22)

    refute_equal deep, back
  end

  def test_long_number_in_listing_ends_with_ellipsis
    text = render(["X 123456789012345678901234567890123456789,5"], width: 80, height: 22)

    assert_includes text, "12345678901234567890…}"
    assert_fits text, 80, 22
  end
end
