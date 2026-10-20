# frozen_string_literal: true

require "test_helper"

class TestSafeLambda < Minitest::Test
  def compile(text)
    SetCalc::SafeLambda.compile(text)
  end

  def value_of(text, input)
    outcome = compile(text).call(input)

    assert_predicate outcome, :ok?, outcome.error
    outcome.text
  end

  def test_accepted_forms
    assert_equal "7", value_of("->(x) { x + 2 }", 5)
    assert_equal "7", value_of("-> (value){value + 2}", 5)
    assert_equal "10", value_of("-> x { x * 2 }", 5)
    assert_equal "10", value_of("lambda { |x| x * 2 }", 5)
    assert_equal "10", value_of("lambda do |x| x * 2 end", 5)
  end

  def test_allowed_expressions
    assert_equal "\"odd\"", value_of("->(x) { x.odd? ? 'odd' : 'even' }", 5)
    assert_equal "3.0", value_of("->(x) { Math.sqrt(x) }", 9)
    assert_equal "6.283", value_of("->(x) { (Math::PI * x).round(3) }", 2)
    assert_equal "[4, 16]", value_of("->(x) { [x, x ** 2] }", 4)
    assert_equal "\"n=4\"", value_of("->(x) { \"n=\#{x}\" }", 4)
    assert_equal "true", value_of("->(x) { x > 1 && !x.zero? }", 4)
    assert_equal "nil", value_of("->(x) { }", 4)
  end

  def test_rejects_texts_that_are_not_one_lambda
    ["1 + 2", "->(x) { x }; ->(y) { y }", "proc { |x| x }", "foo", "->(x) {", ""].each do |text|
      assert_raises(SetCalc::SafeLambda::Error, text) { compile(text) }
    end
  end

  def test_rejects_wrong_parameters
    ["-> { 1 }", "->(x, y) { x }", "->(x = 1) { x }", "->(*x) { x }", "->(x:) { x }",
     "->(x, &b) { x }", "->(x; y) { x }", "-> { it * 2 }", "-> { _1 * 2 }"].each do |text|
      assert_raises(SetCalc::SafeLambda::Error, text) { compile(text) }
    end
  end

  def test_rejects_dangerous_bodies
    [
      "->(x) { system('ls') }", "->(x) { `ls` }", "->(x) { %x(ls) }", "->(x) { exit }",
      "->(x) { eval('1') }", "->(x) { x.send(:exit) }", "->(x) { x.instance_eval { 1 } }",
      "->(x) { File.read('/etc/passwd') }", "->(x) { Kernel.exit }", "->(x) { ::Object }",
      "->(x) { $stdout }", "->(x) { @a }", "->(x) { y = x }", "->(x) { x.times { } }",
      "->(x) { while true; end }", "->(x) { x.method(:abs) }", "->(x) { x.class }",
      "->(x) { Math.const_get(:Foo) }", "->(x) { __method__ }", "->(x) { ObjectSpace }",
      "->(x) { x.public_send(:abs) }", "->(x) { [*x] }", "->(x) { x.abs(&:foo) }",
      "->(x) { BEGIN { } }", "->(x) { def foo; end }", "->(x) { ->(y) { y } }",
      "->(x) { /a/ =~ 'a' }", "->(x) { x.then { 1 } }", "->(x) { (1..x).to_a }"
    ].each do |text|
      assert_raises(SetCalc::SafeLambda::Error, text) { compile(text) }
    end
  end

  def test_sandbox_has_no_kernel
    sandbox = SetCalc::SafeLambda::Sandbox.new.__binding

    assert_raises(NoMethodError) { Kernel.eval("puts 1", sandbox, __FILE__, __LINE__) }
  end

  def test_errors_are_outcomes
    outcome = compile("->(x) { 1 / x }").call(0)

    refute_predicate outcome, :ok?
    assert_match(/ZeroDivisionError/, outcome.error)
  end

  def test_call_all_in_child_process
    outcomes = compile("->(x) { x * 10 }").call_all([1, 2, 3])

    assert_equal %w[10 20 30], outcomes.map(&:text)
    assert_equal [1, 2, 3], outcomes.map(&:input)
  end

  def test_call_all_stops_a_slow_lambda
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    error = assert_raises(SetCalc::SafeLambda::Error) do
      compile("->(x) { (x ** 30_000_000).digits.size }").call_all([7, 7, 7], time_limit: 0.3)
    end

    assert_match(/took more than/, error.message)
    assert_operator Process.clock_gettime(Process::CLOCK_MONOTONIC) - started, :<, 5
  end

  def test_long_results_are_cut
    text = value_of("->(x) { x.to_s * 1000 }", 1)

    assert_operator text.length, :<=, SetCalc::SafeLambda::TEXT_LIMIT + 3
  end
end
