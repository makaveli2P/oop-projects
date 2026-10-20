# frozen_string_literal: true

require "test_helper"

class TestTUIRoute < Minitest::Test
  def routes(source, step_index)
    m = machine(source)
    step_index.times { m.step }
    ops = m.step.micro_ops
    ops.each_index.map { ALI::TUI::Routing.route_of(ops, it) }
  end

  def pairs(list) = list.map { [it.sources, it.targets] }

  def test_lda
    assert_equal [[%i[PC Memory], [:IR]], [[:IR], [:Decoder]], [[:Memory], [:A]], [[:Memory], [:A]], [[], [:PC]]],
                 pairs(routes("DEC X\nLDA X\nHLT", 1))
  end

  def test_ldb_goes_to_b
    assert_equal [[:Memory], [:B]], pairs(routes("DEC X\nLDB X\nHLT", 1))[2]
  end

  def test_ldi_comes_from_the_decoder
    route = routes("LDI 7\nHLT", 0)[2]

    assert_equal [[:Decoder], [:A], "7"], [route.sources, route.targets, route.label]
  end

  def test_str
    assert_equal [[[:A], [:Memory]], [[:A], [:Memory]]], pairs(routes("DEC X\nSTR X\nHLT", 1))[2, 2]
  end

  def test_xch
    assert_equal [[[:A], [:B]], [[:B], [:A]], [[:B], [:A]], [[:A], [:B]]], pairs(routes("XCH\nHLT", 0))[2, 4]
  end

  def test_add
    list = pairs(routes("ADD\nHLT", 0))

    assert_equal [[[:A], [:ALU]], [[:B], [:ALU]], [%i[A B], [:ALU]], [[:ALU], [:A]], [[:ALU], [:Z]], [[:ALU], [:V]]],
                 list[2, 6]
  end

  def test_jumps_and_branches
    assert_equal [[:Decoder], [:PC]], pairs(routes("JMP 0", 0)).last
    branch = routes("JZS 1\nHLT", 0)

    assert_equal [[:Z], [:Decoder], "Z = 0"], [branch[2].sources, branch[2].targets, branch[2].label]
    assert_equal [[], [:PC]], [branch[3].sources, branch[3].targets]
  end

  def test_dec_and_halt
    assert_equal [[:Decoder], [:Memory]], pairs(routes("DEC X\nHLT", 0))[2]
    assert_predicate routes("HLT", 0).last, :empty?
  end

  def test_hex_labels
    m = machine("LDI -1\nHLT")
    ops = m.step.micro_ops

    assert_equal "0xFFFFFFFF", ALI::TUI::Routing.route_of(ops, 2, :hex).label
    assert_equal "0x01", ALI::TUI::Routing.route_of(ops, 3, :hex).label
  end
end
