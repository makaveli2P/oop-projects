# frozen_string_literal: true

require "test_helper"

class TestMachine < Minitest::Test
  def test_initial_state
    m = machine("DEC X\nHLT")

    assert_equal [0, 0, 0, false, false, nil, 0], [m.a, m.b, m.pc, m.z, m.v, m.halt_reason, m.steps]
    assert_equal ALI::MemoryMap::SIZE, m.state.memory.size
    assert_kind_of ALI::CodeLine, m.state.memory[0]
    assert_nil m.state.memory[2]
    assert_equal 0, m.data(255)
    assert_nil m.data(0)
  end

  def test_running_past_the_end_halts
    m = machine("LDI 1\nLDI 2")
    m.step
    step = m.step

    assert_equal %w[Fetch Decode RegWrite PcWrite Halt], op_classes(step)
    assert_equal :ran_past_end, m.halt_reason
    assert_equal 2, m.pc
  end

  def test_a_step_on_a_halted_machine_does_nothing
    m = machine("HLT")
    m.step
    before = m.state
    step = m.step

    refute_predicate step, :executed?
    assert_same before, m.state
  end

  def test_run_stops_at_the_step_limit
    m = machine("JMP 0")
    result = m.run(limit: 1000)

    assert_equal ALI::RunResult.new(steps: 1000, stop: :step_limit), result
    refute_predicate m, :halted?
    assert_equal 1000, m.steps
  end

  def test_run_continues_after_the_limit
    m = ALI::Machine.new(ALI::Assembler.assemble_file(root_path("test2.txt")).program)

    assert_equal :step_limit, m.run.stop
    assert_equal ALI::RunResult.new(steps: 154, stop: :halted), m.run
  end

  def test_states_are_immutable_snapshots
    m = machine("DEC X\nLDI 5\nSTR X\nHLT")
    first = m.state
    3.times { m.step }

    assert_equal 0, first.memory[128]
    assert_equal 5, m.state.memory[128]
    assert_predicate first.memory, :frozen?
    m.restore(first)

    assert_equal 0, m.value_of("X")
    assert_equal 0, m.pc
  end

  def test_reset
    m = machine("LDI 5\nHLT")
    m.run
    m.reset

    assert_equal [0, 0, nil], [m.a, m.pc, m.halt_reason]
  end

  def test_a_fault_halts_the_machine
    # The assembler cannot make this program; the public API can.
    code = [ALI::CodeLine.new(instruction: ALI::Lda.new("X", 5), text: "LDA X", line: 1)]
    m = ALI::Machine.new(ALI::Program.new(code, []))
    step = m.step

    assert_equal %w[Fetch Decode PcWrite Halt], op_classes(step)
    assert_equal :runtime_fault, m.halt_reason
    assert_equal "Halt: runtime fault (address 5 is not a data address)", step.micro_ops.last.to_s
    assert_equal 0, m.a
  end

  def test_program_checks_its_invariants
    hlt = ALI::CodeLine.new(instruction: ALI::Hlt.new, text: "HLT", line: 1)

    assert_raises(ArgumentError) { ALI::Program.new([], []) }
    assert_raises(ArgumentError) { ALI::Program.new([hlt] * 129, []) }
    assert_raises(ArgumentError) { ALI::Program.new([hlt], [ALI::SymbolBinding.new(name: "X", address: 200)]) }
    assert_raises(ArgumentError) do
      ALI::Program.new([hlt], [ALI::SymbolBinding.new(name: "X", address: 128),
                               ALI::SymbolBinding.new(name: "X", address: 129)])
    end
  end

  def test_micro_ops_rebuild_the_state
    m = machine("DEC X\nLDI 7\nSTR X\nXCH\nLDA X\nADD\nJVS 0\nHLT")
    until m.halted?
      before = m.state
      step = m.step
      rebuilt = step.micro_ops.reduce(before) { |state, op| op.apply(state) }

      assert_equal m.state.with(steps: 0), rebuilt.with(steps: 0)
    end
  end
end
