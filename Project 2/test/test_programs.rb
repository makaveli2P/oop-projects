# frozen_string_literal: true

require "test_helper"

# The golden results. They match the C++ version in "Project 4".
class TestPrograms < Minitest::Test
  def run_file(*path)
    loaded = ALI::Assembler.assemble_file(root_path(*path))

    assert_predicate loaded, :ok?
    m = ALI::Machine.new(loaded.program)
    result = m.run(limit: 100_000)

    assert_equal :halted, result.stop
    [m, result.steps]
  end

  def test_first_program_overflows_and_jumps_to_hlt
    m, steps = run_file("test1.txt")

    assert_equal [-1_293_967_296, 1_000_000_000, false, true], [m.a, m.b, m.z, m.v]
    assert_equal [2_001_000_000, 1_000_000_000], [m.value_of("INITIAL"), m.value_of("ADDER")]
    assert_equal [:hlt_executed, 12, 20], [m.halt_reason, m.pc, steps]
  end

  def test_second_program_divides_by_three
    m, steps = run_file("test2.txt")

    assert_equal([114, 0, -3, 1], %w[RESULT INITIAL SUB INCREMENTOR].map { m.value_of(it) })
    assert_equal [true, false, 1154], [m.z, m.v, steps]
  end

  def test_third_program_computes_fibonacci
    m, steps = run_file("test3.txt")

    assert_equal([267_914_296, 433_494_437, 0], %w[PREV RESULT NTHTERM].map { m.value_of(it) })
    assert_equal [:hlt_executed, 522], [m.halt_reason, steps]
  end

  def test_program_copies_match_the_originals
    %w[test1 test2 test3].each do |name|
      assert_equal File.read(root_path("#{name}.txt")), File.read(root_path("programs", "#{name}.sal"))
    end
  end

  def test_countdown
    m, = run_file("programs", "countdown.sal")

    assert_equal [0, 5], [m.value_of("COUNT"), m.value_of("LOOPS")]
  end

  def test_overflow
    m, = run_file("programs", "overflow.sal")

    assert_equal [ALI::Word::MIN, 1], [m.value_of("SUM"), m.value_of("FLAG")]
  end

  def test_swap_runs_past_the_end
    m, = run_file("programs", "swap.sal")

    assert_equal [20, 10, :ran_past_end], [m.value_of("FIRST"), m.value_of("SECOND"), m.halt_reason]
  end
end
