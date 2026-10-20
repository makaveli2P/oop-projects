# frozen_string_literal: true

require "test_helper"

class TestWord < Minitest::Test
  def test_wrap_keeps_values_in_range
    assert_equal 0, ALI::Word.wrap(0)
    assert_equal ALI::Word::MAX, ALI::Word.wrap(ALI::Word::MAX)
    assert_equal ALI::Word::MIN, ALI::Word.wrap(ALI::Word::MAX + 1)
    assert_equal ALI::Word::MAX, ALI::Word.wrap(ALI::Word::MIN - 1)
    assert_equal(-1_293_967_296, ALI::Word.wrap(3_001_000_000))
  end

  def test_hex_format_uses_twos_complement
    assert_equal "0xFFFFFFFF", ALI::Word.format(-1, :hex)
    assert_equal "0x80000000", ALI::Word.format(ALI::Word::MIN, :hex)
    assert_equal "42", ALI::Word.format(42)
  end

  def test_add_max_plus_one_overflows
    sum = ALI::ALU.add(ALI::Word::MAX, 1)

    assert_equal ALI::Word::MIN, sum.result
    assert sum.overflow
    refute sum.zero
  end

  def test_add_min_minus_one_overflows
    sum = ALI::ALU.add(ALI::Word::MIN, -1)

    assert_equal ALI::Word::MAX, sum.result
    assert sum.overflow
  end

  def test_add_min_plus_min_wraps_to_zero_and_sets_both_flags
    sum = ALI::ALU.add(ALI::Word::MIN, ALI::Word::MIN)

    assert_equal 0, sum.result
    assert sum.zero
    assert sum.overflow
  end

  def test_add_legal_results_clear_overflow
    refute ALI::ALU.add(ALI::Word::MIN, 0).overflow
    refute ALI::ALU.add(-2_147_483_647, -1).overflow
    assert ALI::ALU.add(-1, 1).zero
    assert_equal 2_147_483_648, ALI::ALU.add(ALI::Word::MAX, 1).wide_sum
  end

  def test_format_wide_and_address
    assert_equal "3001000000", ALI::Word.format_wide(3_001_000_000)
    assert_equal "0xB2DFA040", ALI::Word.format_wide(3_001_000_000, :hex)
    assert_equal "0x100000000", ALI::Word.format_wide(2**32, :hex)
    assert_equal "-0x80000001", ALI::Word.format_wide(-2_147_483_649, :hex)
    assert_equal "0xFFFFFFFF", ALI::Word.format_wide(-1, :hex)
    assert_equal "0x0C", ALI::Word.format_address(12, :hex)
    assert_equal "12", ALI::Word.format_address(12)
  end

  def test_micro_op_text_in_each_base
    write = ALI::MicroOp::RegWrite.new(reg: "A", old_value: 255, new_value: -1)

    assert_equal "Write A: 255 -> -1", write.to_s
    assert_equal "Write A: 0x000000FF -> 0xFFFFFFFF", write.to_s(:hex)
    pc = ALI::MicroOp::PcWrite.new(old_value: 7, new_value: 12, cause: :jump)

    assert_equal "PC: 0x07 -> 0x0C (jump)", pc.to_s(:hex)
    assert_equal "Test Z: 1", ALI::MicroOp::FlagRead.new(flag: "Z", value: true).to_s(:hex)
  end
end
