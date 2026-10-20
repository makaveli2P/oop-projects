# frozen_string_literal: true

require "test_helper"

class TestTUIController < Minitest::Test
  def controller(source = File.read(root_path("test1.txt")), **)
    ALI::TUI::Controller.new(assemble(source), **)
  end

  def test_starts_ready
    c = controller

    refute_predicate c, :instruction?
    assert_nil c.active_micro_op
    assert_equal [0, 0], [c.cursor, c.step_number]
  end

  def test_next_micro_op_walks_one_instruction
    c = controller("LDI 5\nHLT")
    c.next_micro_op

    assert_kind_of ALI::MicroOp::Fetch, c.active_micro_op
    assert_equal 0, c.view.a
    c.next_micro_op
    c.next_micro_op

    assert_kind_of ALI::MicroOp::RegWrite, c.active_micro_op
    assert_equal 5, c.view.a
    assert_equal 0, c.view.pc
    c.next_micro_op

    assert_predicate c, :instruction_complete?
    assert_equal 1, c.view.pc
    c.next_micro_op

    assert_equal [2, 1], [c.step_number, c.cursor]
  end

  def test_step_completes_the_current_instruction_first
    c = controller("LDI 5\nLDI 6\nHLT")
    c.next_micro_op
    c.step_instruction

    assert_equal [1, 5], [c.step_number, c.view.a]
    c.step_instruction

    assert_equal [2, 6], [c.step_number, c.view.a]
  end

  def test_back_restores_the_previous_instruction
    c = controller("LDI 5\nLDI 6\nHLT")
    c.step_instruction
    c.step_instruction
    c.back

    assert_equal [5, 1, 1], [c.view.a, c.view.pc, c.step_number]
    c.back
    c.back

    assert_equal "This is the start of the program.", c.message
    assert_equal 0, c.view.a
  end

  def test_history_limit
    c = controller("JMP 0", history_limit: 3)
    5.times { c.step_instruction }
    3.times { c.back }

    assert_equal 2, c.view.steps
    c.back

    assert_equal "The history holds no earlier steps.", c.message
  end

  def test_run_to_end_stops_at_a_breakpoint_and_not_where_it_starts
    c = controller
    c.toggle_breakpoint(7)

    assert_equal :breakpoint, c.run_to_end
    assert_equal 7, c.view.pc
    assert_equal "Stopped at the breakpoint at address 7.", c.message
    assert_equal :breakpoint, c.run_to_end
    assert_equal 7, c.view.pc
    assert_equal 12, c.step_number
  end

  def test_run_to_end_halts
    c = controller

    assert_equal :halted, c.run_to_end
    assert_equal :hlt_executed, c.halt_reason
    assert_equal :already_halted, c.run_to_end
    assert_match(/Press R to reset/, c.message)
  end

  def test_run_to_end_step_limit
    c = controller("JMP 0")

    assert_equal :step_limit, c.run_to_end(limit: 1000)
    assert_equal 1000, c.step_number
    assert_match(/Press g to run 1000 more/, c.message)
  end

  def test_animated_run_ticks_micro_ops_and_stops_at_halt
    c = controller("LDI 1\nHLT")
    c.toggle_run

    assert_predicate c, :running?
    ticks = 0
    ticks += 1 while c.tick

    refute_predicate c, :running?
    assert_equal 8, ticks # 4 + 4 micro-operations.
    assert_predicate c, :halted?
  end

  def test_animated_run_stops_before_a_breakpoint
    c = controller("LDI 1\nLDI 2\nLDI 3\nHLT")
    c.toggle_breakpoint(2)
    c.toggle_run
    nil while c.tick

    assert_equal [2, 2], [c.view.pc, c.view.a]
    assert_equal "Stopped at the breakpoint at address 2.", c.message
    c.toggle_run
    c.tick

    assert_equal 3, c.step_number # It does not stop at the breakpoint where it starts.
  end

  def test_toggle_run_on_a_halted_machine
    c = controller("HLT")
    c.step_instruction
    c.toggle_run

    refute_predicate c, :running?
    refute_predicate c, :tick
  end

  def test_speed_levels
    c = controller

    assert_equal "5", c.speed_text
    10.times { c.faster }

    assert_equal ["50", 0.02], [c.speed_text, c.tick_interval]
    10.times { c.slower }

    assert_equal "1", c.speed_text
    c.faster

    assert_equal "1.7", c.speed_text
  end

  def test_selection_and_breakpoints
    c = controller("LDI 1\nHLT")
    c.move_selection(5)

    assert_equal 1, c.selection
    c.toggle_breakpoint

    assert c.breakpoint?(1)
    c.toggle_breakpoint(1)

    refute c.breakpoint?(1)
    c.toggle_breakpoint(9)

    assert_empty c.breakpoints
  end

  def test_reset_keeps_breakpoints
    c = controller
    c.toggle_breakpoint(3)
    c.run_to_end
    c.reset

    assert_equal [0, 0, true], [c.view.pc, c.history_size, c.breakpoint?(3)]
    assert_equal "The machine is reset.", c.message
  end
end
