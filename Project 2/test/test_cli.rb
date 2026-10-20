# frozen_string_literal: true

require "test_helper"

class TestCLI < Minitest::Test
  def cli(*argv, input: "")
    out = StringIO.new
    err = StringIO.new
    status = Dir.chdir(root_path) do
      ALI::CLI.start(argv, input: StringIO.new(input), output: out, error: err)
    end
    [status, out.string, err.string]
  end

  def test_plain_mode_with_a_file
    status, out, = cli("--plain", "test3", input: "a\nq\n")

    assert_equal 0, status
    assert_includes out, "Executed 522 steps."
  end

  def test_render_prints_one_frame
    status, out, = cli("--render", "programs/test1.sal", "--steps", "3", "--size", "100x30")

    assert_equal 0, status
    assert_equal 30, out.lines.size
    assert_includes out, "Datapath"
  end

  def test_render_of_a_bad_program_exits_with_status_one
    Dir.mktmpdir do |dir|
      path = File.join(dir, "bad.sal")
      File.write(path, "JMP 5\n")
      status, out, = cli("--render", path)

      assert_equal 1, status
      assert_includes out, "jump target '5' is not an instruction address (0-0)"
    end
  end

  def test_the_visualizer_needs_a_terminal
    status, _, err = cli("programs/test1.sal")

    assert_equal 1, status
    assert_includes err, "Use --plain"
  end

  def test_usage_errors
    assert_equal 2, cli("--render").first
    assert_equal 2, cli("--bogus").first
    assert_equal 2, cli("a", "b").first
    assert_equal 2, cli("--render", "x.sal", "--size", "big").first
    assert_equal 2, cli("--render", "x.sal", "--steps", "-1").first
  end

  def test_help_and_version
    assert_includes cli("--help")[1], "Usage: ali"
    assert_equal "ali #{ALI::VERSION}\n", cli("--version")[1]
  end
end
