# frozen_string_literal: true

require "test_helper"

class TestCLI < Minitest::Test
  def run_cli(argv, input: "")
    out = StringIO.new
    err = StringIO.new
    code = SetCalc::CLI.run(argv, input: StringIO.new(input), output: out, error: err)
    [code, out.string, err.string]
  end

  def test_classic_mode_with_piped_input
    code, out, = run_cli(["--plain"], input: "X 5,3,8\na 4\nl ->(x) { x + 1 }\nfoo\nq\nX 1\n")

    assert_equal 0, code
    assert_equal <<~TEXT, out
      X = {3, 5, 8}
      Y = {}
      Z = {}
      X = {3, 4, 5, 8}
      Y = {}
      Z = {}
      [4, 5, 6, 9]
      X = {3, 4, 5, 8}
      Y = {}
      Z = {}
      error: unknown command "foo". The commands are: X values, Y values, Z values, a i, r, s, u, i, c, l aString, q
    TEXT
  end

  def test_no_terminal_falls_back_to_classic
    _, out, = run_cli([], input: "Z 2\n")

    assert_includes out, "Z = {2}"
  end

  def test_end_of_input_quits
    code, out, = run_cli(["--plain"], input: "X 1")

    assert_equal 0, code
    assert_includes out, "X = {1}"
  end

  def test_classic_prompt_on_a_terminal
    out = StringIO.new
    SetCalc::Classic.new(input: StringIO.new("q\n"), output: out, prompt: true).run

    assert_includes out.string, "Commands:"
    assert_includes out.string, "> "
  end

  def test_render
    code, out, = run_cli(["--render", "--cmd", "X 5,3", "--cmd", "a 4", "--step", "2", "--size", "90x24"])

    assert_equal 0, code
    assert_includes out, "4 < 5, go left."
    assert_equal 24, out.lines.size
  end

  def test_bad_options
    code, _, err = run_cli(["--size", "big"])

    assert_equal 2, code
    assert_includes err, "Usage:"
    assert_equal 2, run_cli(["--render", "--keys", "<nope>"]).first
  end

  def test_help
    code, out, = run_cli(["--help"])

    assert_equal 0, code
    assert_includes out, "--plain"
  end

  def test_classic_mode_survives_invalid_utf8
    code, out, = run_cli(["--plain"], input: (+"X 1\xff,2\na 3\nq\n").force_encoding(Encoding::UTF_8))

    assert_equal 0, code
    assert_equal "error: \"1?\" is not a number\nX = {3}\nY = {}\nZ = {}\n", out
  end
end
