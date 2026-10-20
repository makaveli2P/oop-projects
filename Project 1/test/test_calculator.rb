# frozen_string_literal: true

require "test_helper"

class TestCalculator < Minitest::Test
  def sets(calc)
    %i[X Y Z].map { |name| calc[name].to_a }
  end

  def test_sets_start_empty
    assert_equal [[], [], []], sets(SetCalc::Calculator.new)
  end

  def test_assign_replaces_the_set
    calc = calculator_after("X 5,3,8", "X 1,2")

    assert_equal [1, 2], calc[:X].to_a
  end

  def test_assign_reports_duplicates
    result = SetCalc::Calculator.new.run("X 1,2,1")

    assert_equal ["1 is a duplicate. X has it only once."], result.output
  end

  def test_add_inserts_into_x
    calc = calculator_after("X 5,3", "a 4", "a 4")

    assert_equal [3, 4, 5], calc[:X].to_a
  end

  def test_add_into_empty_x
    assert_equal [7], calculator_after("a 7")[:X].to_a
  end

  def test_rotate
    calc = calculator_after("X 1", "Y 2", "Z 3", "r")

    assert_equal [[3], [1], [2]], sets(calc)
  end

  def test_switch
    calc = calculator_after("X 1", "Y 2", "Z 3", "s")

    assert_equal [[2], [1], [3]], sets(calc)
  end

  def test_union
    calc = calculator_after("X 5,3,8", "Y 1,8,10", "u")

    assert_equal [1, 3, 5, 8, 10], calc[:X].to_a
    assert_equal [1, 8, 10], calc[:Y].to_a
  end

  def test_union_with_empty_sets
    assert_equal [1], calculator_after("X 1", "u")[:X].to_a
    assert_equal [2], calculator_after("Y 2", "u")[:X].to_a
  end

  def test_intersection
    calc = calculator_after("X 5,3,8,1", "Y 1,8,10", "i")

    assert_equal [1, 8], calc[:X].to_a
    assert_equal [1, 8, 10], calc[:Y].to_a
  end

  def test_intersection_keeps_the_preorder_shape_of_x
    calc = calculator_after("X 5,3,8,1,4", "Y 4,1,5,3", "i")

    assert_equal [5, 3, 1, 4], calc[:X].each_preorder.to_a
  end

  def test_intersection_with_empty_y
    assert_empty calculator_after("X 1,2", "i")[:X]
  end

  def test_deep_copy_is_independent
    calc = calculator_after("X 5,3,8", "Y 99", "c", "a 4")

    assert_equal [3, 5, 8], calc[:Y].to_a
    assert_equal [3, 4, 5, 8], calc[:X].to_a
    refute_same calc[:X].root, calc[:Y].root
  end

  def test_rotate_then_switch_keeps_trees_independent
    calc = calculator_after("X 1", "c", "a 2")

    assert_equal [[1, 2], [1], []], sets(calc)
  end

  def test_lambda_prints_results_and_keeps_x
    calc = calculator_after("X 5,3,8")
    result = calc.run("l -> (value){value + 2}")

    assert_equal ["[5, 7, 10]"], result.output
    assert_equal [3, 5, 8], calc[:X].to_a
  end

  def test_lambda_errors_are_shown_for_each_element
    calc = calculator_after("X 0,1")
    result = calc.run("l ->(x) { 10 / x }")

    refute result.error
    assert_equal "[<ZeroDivisionError>, 10]", result.output.first
    assert_match(/f\(0\) raised ZeroDivisionError/, result.output.last)
  end

  def test_rejected_lambda_is_an_error_result
    result = calculator_after("X 1").run("l ->(x) { system('ls') }")

    assert result.error
    assert_match(/not allowed/, result.summary)
  end

  def test_quit
    assert SetCalc::Calculator.new.run("q").quit
  end

  def test_bad_input_never_raises
    calc = SetCalc::Calculator.new

    ["", "X a,b", "a", "zz", "l", "l 1 +", "u 1"].each do |line|
      assert calc.run(line).error, "expected an error for #{line.inspect}"
    end
  end

  def test_size_limit
    calc = SetCalc::Calculator.new
    limit = SetCalc::Calculator::MAX_ELEMENTS

    assert calc.run("X #{(1..(limit + 1)).to_a.join(',')}").error
    refute calc.run("X #{(1..limit).to_a.join(',')}").error
    assert calc.run("a 0").error
    assert_predicate calc.run("a 1").output, :any?
    calc.run("Y 0")

    assert calc.run("u").error
    assert_equal limit, calc[:X].size
  end

  def test_size_limit_stops_early_for_a_long_sorted_list
    calc = SetCalc::Calculator.new
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    result = calc.run("X #{(1..30_000).to_a.join(',')}")
    elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started

    assert result.error
    assert_operator elapsed, :<, 5, "the limit check must stop at the first value past the limit"
    refute calc.run("X #{([1, 1.0] * 5000).join(',')}").error, "1 and 1.0 are one element"
  end

  def test_lambda_error_label_keeps_the_namespace
    calc = calculator_after("X 0,3,-4")
    result = calc.run("l ->(x) { Math.sqrt(x) }")

    assert_equal "[<Math::DomainError>, 0.0, 1.7320508075688772]", result.output.first
  end
end
