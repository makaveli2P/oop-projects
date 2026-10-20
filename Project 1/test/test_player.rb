# frozen_string_literal: true

require "test_helper"

class TestPlayer < Minitest::Test
  def player
    @player ||= SetCalc::TUI::Player.new
  end

  def type(text)
    text.each_char { |char| player.press(char) }
  end

  def test_starts_ready_with_one_frame
    assert_equal "Ready", player.state
    assert_equal 1, player.frames.size
    refute_predicate player, :playing?
  end

  def test_enter_runs_the_command_and_plays
    type("X 5,3,8")
    player.press(:enter)

    assert_predicate player, :playing?
    assert_equal 0, player.index
    assert_equal "", player.input
    assert_equal [3, 5, 8], player.calculator[:X].to_a
  end

  def test_tick_plays_to_the_end
    player.submit("X 5,3")
    20.times { player.tick }

    assert_predicate player, :at_end?
    refute_predicate player, :playing?
    assert_equal "Done", player.state
  end

  def test_space_steps_only_with_an_empty_input
    player.submit("X 5,3")
    player.press(" ")

    assert_equal 1, player.index
    refute_predicate player, :playing?
    type("a 4")

    assert_equal "a 4", player.input
    assert_equal 1, player.index
  end

  def test_back_and_bounds
    player.submit("X 5,3")
    player.press(:left)

    assert_equal 0, player.index
    player.press(:end)
    player.press(:right)

    assert_predicate player, :at_end?
    player.press(:left)

    assert_equal player.frames.size - 2, player.index
  end

  def test_tab_toggles_and_replays_from_the_start
    player.submit("X 5,3")
    player.press(:tab)

    refute_predicate player, :playing?
    player.press(:end)
    player.press(:tab)

    assert_predicate player, :playing?
    assert_equal 0, player.index
  end

  def test_speed_limits
    10.times { player.press("+") }

    assert_equal SetCalc::TUI::Player::SPEEDS.last, player.speed
    20.times { player.press("-") }

    assert_equal 1, player.speed
  end

  def test_minus_types_inside_a_command
    type("a -5")
    player.press(:enter)

    assert_equal [-5], player.calculator[:X].to_a
  end

  def test_error_keeps_the_frames_and_shows_at_once
    player.submit("X 5,3")
    frames = player.frames
    player.submit("X 1,zz")

    assert_same frames, player.frames
    assert player.message.error
  end

  def test_message_waits_for_the_last_frame
    player.submit("X 5")

    assert_nil player.message
    player.press(:end)

    assert_equal "X = {5}", player.message.summary
  end

  def test_backspace_escape_and_quit
    type("ab")
    player.press(:backspace)

    assert_equal "a", player.input
    player.press(:escape)

    assert_equal "", player.input
    refute_predicate player, :quit?
    player.press(:escape)

    assert_predicate player, :quit?
  end

  def test_q_command_quits
    player.submit("q")

    assert_predicate player, :quit?
  end

  def test_scroll_resets_on_a_new_frame
    player.submit("X 1,2,3")
    player.press(:down)

    assert_equal 2, player.scroll
    player.press(:right)

    assert_equal 0, player.scroll
  end

  def test_rejects_non_ascii_input
    player.press("é")

    assert_equal "", player.input
  end

  def test_scroll_stays_in_the_range_that_the_screen_reports
    player.submit("X #{(1..20).to_a.join(',')}")
    player.press(:end)
    SetCalc::TUI::Screen.render(player, 80, 22)
    20.times { player.press(:down) }
    bottom = player.scroll
    SetCalc::TUI::Screen.render(player, 80, 22)
    player.press(:up)

    assert_equal bottom - 2, player.scroll
    20.times { player.press(:up) }
    top = player.scroll
    player.press(:down)

    assert_equal top + 2, player.scroll
  end
end
