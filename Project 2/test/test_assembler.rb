# frozen_string_literal: true

require "test_helper"

class TestAssembler < Minitest::Test
  def test_symbols_get_data_addresses_in_order
    program = assemble("DEC X\nDEC Y\nHLT\n")

    assert_equal([["X", 128], ["Y", 129]], program.symbols.map { [it.name, it.address] })
    assert_equal 3, program.size
  end

  def test_blank_and_comment_lines_take_no_address
    program = assemble("; header\n\nDEC X   # declare\n   \nLDA X ; load\nHLT\n")

    assert_equal ["DEC X", "LDA X", "HLT"], program.code.map(&:text)
    assert_equal [3, 5, 6], program.code.map(&:line)
  end

  def test_opcodes_are_case_insensitive_and_symbols_are_not
    program = assemble("dec x\nDEC X\nLda x\nldb X\nhlt\n")

    assert_equal(%w[DEC DEC LDA LDB HLT], program.code.map { it.instruction.mnemonic })
    assert_equal [128, 129], [program.at(2).instruction.address, program.at(3).instruction.address]
  end

  def test_crlf_bom_and_trailing_spaces
    program = assemble("﻿DEC X \r\nLDI 5\t\r\nSTR X\r\n")

    assert_equal ["DEC X", "LDI 5", "STR X"], program.code.map(&:text)
  end

  def test_use_before_declaration_is_legal
    program = assemble("LDA X\nDEC X\n")

    assert_equal 128, program.at(0).instruction.address
  end

  def test_ldi_accepts_the_full_32_bit_range
    program = assemble("LDI -2147483648\nLDI +2147483647\nLDI 007\n")

    assert_equal([ALI::Word::MIN, ALI::Word::MAX, 7], program.code.map { it.instruction.value })
  end

  def test_reports_all_errors_with_line_and_column
    messages = errors("DEC X\nlda Y\nfoo 1\nJMP 99\n\nLDI 99999999999\nADD 1\nDEC X\nSTR\nLDI abc\nJMP -1\nLDA X1\n")
               .map(&:to_s)

    assert_equal [
      "line 2, column 5: undeclared symbol 'Y'",
      "line 3, column 1: unknown opcode 'foo'",
      "line 4, column 5: jump target '99' is not an instruction address (0-10)",
      "line 6, column 5: '99999999999' is not a 32-bit signed integer",
      "line 7, column 5: ADD expects 0 operands, found 1",
      "line 8, column 5: duplicate declaration of symbol 'X'",
      "line 9, column 1: STR expects 1 operand, found 0",
      "line 10, column 5: 'abc' is not a 32-bit signed integer",
      "line 11, column 5: jump target '-1' is not an instruction address (0-10)",
      "line 12, column 5: 'X1' is not a valid symbol (use letters only)"
    ], messages
  end

  def test_ldi_out_of_range_by_one
    assert_equal "line 1, column 5: '2147483648' is not a 32-bit signed integer", errors("LDI 2147483648").first.to_s
    assert_equal "line 1, column 5: '-2147483649' is not a 32-bit signed integer", errors("LDI -2147483649").first.to_s
  end

  def test_empty_program_is_an_error
    assert_equal ["the program contains no instructions"], errors("; nothing\n\n").map(&:to_s)
  end

  def test_more_than_128_instructions_is_an_error
    error = errors("HLT\n" * 129).first

    assert_equal [129, 1], [error.line, error.column]
    assert_match(/more than 128 instructions/, error.message)
  end

  def test_128_instructions_is_legal
    assert_equal 128, assemble("HLT\n" * 128).size
  end

  def test_more_than_128_symbols_is_an_error
    names = ("a".."z").to_a.product(("a".."e").to_a).map(&:join).first(129)
    # 129 DEC lines are also 129 instructions, so the assembler reports both.
    messages = errors(names.map { "DEC #{it}\n" }.join).map(&:to_s)

    assert_equal ["line 129, column 1: the program has more than 128 instructions",
                  "line 129, column 5: the program declares more than 128 symbols"], messages
  end

  def test_missing_file
    result = ALI::Assembler.assemble_file("/no/such/file.sal")

    refute_predicate result, :ok?
    assert_match(/cannot open file/, result.errors.first.to_s)
  end

  def test_invalid_utf8_does_not_crash
    assert_equal "line 1, column 1: unknown opcode '�'", errors("\xFF\n").first.to_s
  end
end
