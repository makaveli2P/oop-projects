# frozen_string_literal: true

require "test_helper"

class TestPlainMode < Minitest::Test
  def run_plain(input, file: nil, step_limit: ALI::Machine::DEFAULT_STEP_LIMIT)
    out = StringIO.new
    err = StringIO.new
    status = Dir.chdir(root_path) do
      ALI::PlainMode.new(input: StringIO.new(input), output: out, error: err, step_limit:).run(file)
    end
    [status, out.string, err.string]
  end

  def test_prompts_for_the_file_and_accepts_the_name_without_extension
    status, out, = run_plain("test1\nq\n")

    assert_equal 0, status
    assert_includes out, "Enter the SAL file name: "
    assert_includes out, "Loaded 13 instructions and 2 symbols from test1.txt."
  end

  def test_accepts_the_name_with_extension
    _, out, = run_plain("test1.txt\nq\n")

    assert_includes out, "from test1.txt."
  end

  def test_asks_again_for_a_missing_file
    status, out, = run_plain("nothing\ntest3\nq\n")

    assert_equal 0, status
    assert_includes out, "There is no file 'nothing'"
    assert_includes out, "from test3.txt."
  end

  def test_end_of_input_at_the_prompt
    status, _, err = run_plain("")

    assert_equal 1, status
    assert_includes err, "no file name"
  end

  def test_step_prints_the_trace_registers_code_and_data
    _, out, = run_plain("s\ns\ns\nq\n", file: "test1.txt")

    assert_includes out, "[step 3] address 2: LDI 1000000  -- Load the value 1000000 into A"
    assert_includes out, "    Write A: 0 -> 1000000"
    assert_includes out, "Registers: A = 1000000  B = 0  PC = 3  Z = 0  V = 0"
    assert_includes out, "  ->   3  STR INITIAL"
    assert_includes out, "     128  INITIAL      = 0"
  end

  def test_run_all_asks_to_continue_after_the_limit
    _, out, = run_plain("a\ny\nq\n", file: "test2.txt")

    assert_includes out, "The program ran 1000 steps and did not halt. Continue? [y/n] "
    assert_includes out, "Executed 1154 steps."
    assert_includes out, "     130  RESULT       = 114"
    assert_includes out, "The machine is halted: HLT executed."
  end

  def test_run_all_stops_when_the_user_says_no
    _, out, = run_plain("a\nn\nq\n", file: "test2.txt")

    assert_includes out, "Executed 1000 steps."
    refute_includes out, "The machine is halted"
  end

  def test_run_all_prints_the_final_state_without_hlt
    _, out, = run_plain("a\nq\n", file: "programs/swap.sal")

    assert_includes out, "The machine is halted: ran past end of program."
    assert_includes out, "     128  FIRST        = 20"
  end

  def test_commands_after_a_halt
    _, out, = run_plain("a\ns\na\nx\n\nq\n", file: "test1.txt")

    assert_equal 2, out.scan("The machine is already halted. Nothing to execute.").size
    assert_includes out, "Unknown command. Use s, a or q."
  end

  def test_end_of_input_quits
    status, = run_plain("s\n", file: "test1.txt")

    assert_equal 0, status
  end

  def test_load_errors_exit_with_status_one
    Dir.mktmpdir do |dir|
      path = File.join(dir, "bad.sal")
      File.write(path, "LDA X\nfoo\n")
      status, _, err = run_plain("", file: path)

      assert_equal 1, status
      assert_includes err, "#{path}: line 1, column 5: undeclared symbol 'X'"
      assert_includes err, "#{path}: line 2, column 1: unknown opcode 'foo'"
    end
  end
end
