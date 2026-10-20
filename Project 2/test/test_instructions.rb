# frozen_string_literal: true

require "test_helper"

class TestInstructions < Minitest::Test
  def run_to(source, steps)
    m = machine(source)
    steps.times { m.step }
    m
  end

  def test_dec_changes_no_data_and_reports_the_address
    m = machine("DEC X\nHLT")
    step = m.step

    assert_equal %w[Fetch Decode SymbolInfo PcWrite], op_classes(step)
    assert_equal "Symbol X is at address 128", step.micro_ops[2].to_s
    assert_equal 0, m.value_of("X")
    assert_equal 1, m.pc
  end

  def test_lda_ldb_ldi_str
    m = run_to("DEC X\nLDI -7\nSTR X\nLDI 0\nLDA X\nLDB X\nHLT", 6)

    assert_equal(-7, m.value_of("X"))
    assert_equal(-7, m.a)
    assert_equal(-7, m.b)
  end

  def test_lda_trace
    step = run_to("DEC X\nLDA X\nHLT", 1).step

    assert_equal %w[Fetch Decode MemRead RegWrite PcWrite], op_classes(step)
    assert_equal "Read Memory[128] (X): 0", step.micro_ops[2].to_s
  end

  def test_str_trace
    step = run_to("DEC X\nLDI 5\nSTR X\nHLT", 2).step

    assert_equal %w[Fetch Decode RegRead MemWrite PcWrite], op_classes(step)
    assert_equal "Write Memory[128] (X): 0 -> 5", step.micro_ops[3].to_s
  end

  def test_xch_swaps_and_traces_both_reads_first
    xch = run_to("LDI 3\nXCH\nHLT", 1).step

    assert_equal %w[Fetch Decode RegRead RegRead RegWrite RegWrite PcWrite], op_classes(xch)
    m = run_to("LDI 3\nXCH\nLDI 4\nHLT", 3)

    assert_equal [4, 3], [m.a, m.b]
  end

  def test_add_trace_and_flags
    m = run_to("LDI 2147483647\nXCH\nLDI 1\nADD\nHLT", 3)
    step = m.step

    assert_equal %w[Fetch Decode RegRead RegRead AluAdd RegWrite FlagWrite FlagWrite PcWrite], op_classes(step)
    assert_equal "ALU: 1 + 2147483647 = 2147483648, wraps to -2147483648", step.micro_ops[4].to_s
    assert_equal ALI::Word::MIN, m.a
    assert m.v
    refute m.z
  end

  def test_add_clears_flags_again
    m = run_to("LDI 1\nXCH\nLDI -1\nADD\nADD\nHLT", 4)

    assert m.z
    m.step

    refute m.z
    assert_equal 1, m.a
  end

  def test_jmp
    m = run_to("JMP 2\nHLT\nLDI 9\nHLT", 1)

    assert_equal 2, m.pc
    assert_equal "PC: 0 -> 2 (jump)", run_to("JMP 2\nHLT\nHLT", 0).step.micro_ops.last.to_s
  end

  def test_jzs_and_jvs_taken_and_not_taken
    not_taken = run_to("JZS 2\nHLT\nHLT", 0).step

    assert_equal %w[Fetch Decode FlagRead PcWrite], op_classes(not_taken)
    assert_equal "PC: 0 -> 1 (branch not taken)", not_taken.micro_ops.last.to_s

    m = run_to("LDI 0\nADD\nJZS 4\nHLT\nHLT", 2)

    assert_equal "PC: 2 -> 4 (branch taken)", m.step.micro_ops.last.to_s

    v = run_to("LDI -2147483648\nXCH\nLDI -1\nADD\nJVS 6\nHLT\nHLT", 5)

    assert_equal 6, v.pc
  end

  def test_hlt_keeps_the_pc
    m = run_to("LDI 1\nHLT\nLDI 2", 1)
    step = m.step

    assert_equal %w[Fetch Decode PcWrite Halt], op_classes(step)
    assert_equal 1, m.pc
    assert_equal :hlt_executed, m.halt_reason
  end

  def test_describe_sentences
    program = assemble("DEC X\nLDA X\nLDB X\nLDI 4\nSTR X\nXCH\nJMP 0\nJZS 1\nJVS 2\nADD\nHLT")

    assert_equal(["Declare symbol X (address 128)", "Load A from X (address 128)", "Load B from X (address 128)",
                  "Load the value 4 into A", "Store A into X (address 128)", "Exchange A and B",
                  "Jump to address 0", "Jump to address 1 if Z is set", "Jump to address 2 if V is set",
                  "Add B to A; set Z and V", "Halt the machine"], program.code.map { it.instruction.describe })
  end

  def test_instruction_set_is_complete
    assert_equal %w[ADD DEC HLT JMP JVS JZS LDA LDB LDI STR XCH], ALI::INSTRUCTION_SET.keys.sort
    assert(ALI::INSTRUCTION_SET.values.all? { it < ALI::Instruction })
  end

  def test_ldi_rejects_a_wide_value
    assert_raises(ArgumentError) { ALI::Ldi.new(2**31) }
  end
end
