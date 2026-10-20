# frozen_string_literal: true

require "test_helper"

class TestTUIKeys < Minitest::Test
  def setup
    @c = ALI::TUI::Controller.new(assemble("LDI 1\nLDI 2\nHLT"))
  end

  def press(*keys) = keys.map { ALI::TUI::Keys.handle(@c, it) }

  def test_space_and_n_show_the_next_micro_op
    press(" ", "n")

    assert_equal 2, @c.cursor
  end

  def test_step_back_reset
    press("s", "s", "b")

    assert_equal 1, @c.step_number
    press("R")

    assert_equal 0, @c.step_number
  end

  def test_run_pause_and_space_pauses_the_run
    press("r")

    assert_predicate @c, :running?
    press(" ")

    refute_predicate @c, :running?
  end

  def test_speed_hex_selection_and_breakpoint
    press("+", "=", "-", "x", "j", "Down", "k", "Enter")

    assert_equal 4, @c.speed_level
    assert_equal :hex, @c.base
    assert @c.breakpoint?(1)
    press("Up", "Enter")

    assert @c.breakpoint?(0)
  end

  def test_run_to_end_quit_and_unknown_keys
    assert_equal %i[handled quit quit ignored], press("g", "q", "Escape", "?")
    assert_predicate @c, :halted?
  end
end
