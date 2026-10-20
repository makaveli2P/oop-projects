# frozen_string_literal: true

require "test_helper"

class TestTUIRender < Minitest::Test
  SIZES = [[100, 30], [120, 36], [140, 45], [200, 60]].freeze

  def frame(file = "programs/test1.sal", **)
    text, status = ALI::TUI::Render.frame(root_path(file), **)

    assert_equal 0, status
    text
  end

  def assert_fits(text, width, height)
    lines = text.lines(chomp: true)

    assert_equal height, lines.size
    lines.each { assert_equal width, it.length, "line too wide or too short: #{it.inspect}" }
  end

  def test_every_frame_of_every_program_fits_each_size
    Dir[root_path("programs", "*.sal")].each do |path|
      loaded = ALI::Assembler.assemble_file(path)
      m = ALI::Machine.new(loaded.program)
      steps = m.run(limit: 60).steps
      SIZES.each do |width, height|
        [0, 1, steps / 2, steps].uniq.each do |count|
          (0..3).each do |ops|
            text, = ALI::TUI::Render.frame(path, steps: count, ops:, size: [width, height])

            assert_fits(text, width, height)
          end
        end
      end
    end
  end

  def test_the_third_add_of_test1
    text = frame(steps: 17, ops: 5)

    assert_includes text, "▸ ALU: 2001000000 + 1000000000 = 3001000000, wraps to -1293967296"
    assert_includes text, "-1293967296 ▶"
    assert_includes text, "step 18  ·  micro-op 5 of 9"
    assert_includes text, "▶   7  ADD"
  end

  def test_ready_screen_shows_the_primary_action
    text = frame

    assert_includes text, "Press space to start."
    assert_includes text, "Ready"
  end

  def test_halt_markers
    assert_includes frame(keys: "g"), "■  12  HLT"
    swap = frame("programs/swap.sal", keys: "g")

    assert_includes swap, "■  12  end"
    assert_includes swap, "Halted: ran past end of program"
  end

  def test_breakpoint_and_selection
    text = frame(keys: "jjjEg")

    assert_includes text, "▶●  3  STR INITIAL"
    assert_includes text, "Stopped at the breakpoint at address 3."
  end

  def test_memory_marks
    read = frame("programs/test2.sal", steps: 13, ops: 3)

    assert_includes read, "▸ 131 INCREMENTOR"
    write = frame("programs/test2.sal", steps: 16, ops: 4)

    assert_includes write, "◂ 130 RESULT"
    dec = frame("programs/test2.sal", ops: 3)

    assert_includes dec, "• 128 INITIAL"
  end

  def test_hex_mode
    text = frame(steps: 20, keys: "x")

    assert_includes text, "A 0xB2DFA040"
    assert_includes text, "hexadecimal  ·  speed"
  end

  # P2R-07: one frame uses one base for each value.
  def test_hex_mode_narration_uses_hex
    write = frame(steps: 17, ops: 6, keys: "x", size: [100, 30])

    assert_includes write, "▸ Write A: 0x7744D640 -> 0xB2DFA040"
    alu = frame(steps: 17, ops: 5, keys: "x", size: [100, 30])

    assert_includes alu, "▸ ALU: 0x7744D640 + 0x3B9ACA00 = 0xB2DFA040, wraps to 0xB2DFA040"
    pc = frame(steps: 17, ops: 9, keys: "x", size: [100, 30])

    assert_includes pc, "▸ PC: 0x07 -> 0x08 (increment)"
  end

  # The cell at each pane border must stay a border character.
  def assert_borders(text)
    lines = text.lines(chomp: true)
    columns = (0...lines[1].length).select { "╮╭".include?(lines[1][it]) }
    lines[2..-6].each do |line|
      columns.each { assert_equal "│", line[it], "pane border overwritten: #{line.inspect}" }
    end
  end

  LONG = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"

  def with_long_symbol(name = LONG)
    Dir.mktmpdir do |dir|
      path = File.join(dir, "long.sal")
      File.write(path, "DEC #{name}\nLDI -2147483648\nSTR #{name}\nHLT\n")
      yield path
    end
  end

  # P2R-01: the start hint and the instruction card stay inside the Datapath pane.
  def test_card_and_hint_stay_inside_the_pane
    SIZES.each { |width, height| assert_borders(frame(size: [width, height])) }
    with_long_symbol do |path|
      SIZES.each do |width, height|
        (0..5).each do |ops|
          text, = ALI::TUI::Render.frame(path, steps: 2, ops:, size: [width, height])

          assert_borders(text)
          assert_fits(text, width, height)
        end
      end
    end
  end

  def test_card_title_ends_before_the_dots
    with_long_symbol(LONG[0, 52]) do |path|
      text, = ALI::TUI::Render.frame(path, steps: 2, ops: 3, size: [100, 30])

      assert_match(/STR ABCDEFGHIJKLMNOPQRSTUVWXYZ… ● ● ● ○ ○ /, text)
      assert_includes text, "(address 128)"
    end
  end

  # P2R-02: the narration shows the complete values.
  def test_narration_keeps_the_values
    text = frame(steps: 17, ops: 5, size: [100, 30])

    assert_includes text, "wraps to -1293967296\n"[0..-2]
    assert_includes text, "step 18  ·  micro-op 5 of 9"
    with_long_symbol do |path|
      text, = ALI::TUI::Render.frame(path, steps: 2, ops: 4, size: [100, 30])
      narration = text.lines.find { it.include?("▸ Write") }

      assert_includes narration, "…): 0 -> -2147483648"
      assert_includes narration, "Memory[128] (ABCDEFGH"
    end
  end

  # P2R-04: --render follows the selection, as the app does.
  def test_render_follows_the_selection
    Dir.mktmpdir do |dir|
      path = File.join(dir, "big.sal")
      File.write(path, "#{(["LDI 0"] * 126).join("\n")}\nHLT\n")
      text, = ALI::TUI::Render.frame(path, keys: "#{"D" * 60}E", size: [100, 30])

      assert_includes text, "│  ● 60  LDI 0"
      refute_includes text, "    0  LDI 0"
    end
  end

  # P2R-03: only a click on the Program pane selects a line.
  def test_click_hit_test_uses_the_program_pane
    loaded = ALI::Assembler.assemble_file(root_path("programs/test1.sal"))
    screen = ALI::TUI::Screen.new(ALI::TUI::Controller.new(loaded.program), "test1.sal")
    canvas = screen.draw(100, 30)
    row = screen.source_rows.fetch(4)

    assert_equal 4, screen.address_at(10, row)
    assert_equal 4, screen.address_at(24, row)
    assert_equal "│", canvas.char_at(24, row)
    assert_nil screen.address_at(25, row)
    assert_nil screen.address_at(28, row)
    screen.draw(120, 36)

    assert_equal 4, screen.address_at(27, screen.source_rows.fetch(4))
    assert_nil screen.address_at(28, screen.source_rows.fetch(4))
  end

  def test_small_terminal
    text, = ALI::TUI::Render.frame(root_path("programs/test1.sal"), size: [80, 24])

    assert_includes text, "ALI needs a terminal of at least 100 × 30."
    assert_fits(text, 80, 24)
  end

  def test_color_output_uses_sgr_codes
    text = frame(steps: 17, ops: 5, color: true)

    assert_includes text, "\e[38;5;214"
    assert_includes text, "\e[0m"
  end

  def test_error_screen
    Dir.mktmpdir do |dir|
      path = File.join(dir, "bad.sal")
      File.write(path, (1..20).map { "BAD#{it}\n" }.join)
      text, status = ALI::TUI::Render.frame(path, size: [100, 30])

      assert_equal 1, status
      assert_includes text, "20 load errors"
      assert_includes text, "line 1, column 1: unknown opcode 'BAD1'"
      assert_match(/… and \d+ more errors/, text)
      assert_fits(text, 100, 30)
    end
  end

  # P2R-06: the singular for one hidden error.
  def test_error_screen_one_more_error
    Dir.mktmpdir do |dir|
      path = File.join(dir, "bad.sal")
      File.write(path, (1..7).map { "BAD#{it}\n" }.join)
      text, = ALI::TUI::Render.frame(path, size: [100, 30])

      assert_includes text, "… and 1 more error. ALI prints"
    end
  end

  def test_many_symbols_scroll_the_memory_pane
    source = (1..60).map { "DEC #{var_name(it)}" } + ["LDI 7"] + (1..60).map { "STR #{var_name(it)}" } + ["HLT"]
    Dir.mktmpdir do |dir|
      path = File.join(dir, "many.sal")
      File.write(path, source.join("\n"))
      text, = ALI::TUI::Render.frame(path, steps: 100, ops: 4, size: [100, 30])

      assert_includes text, "◂ 167 #{var_name(40)}"
      assert_match(/↑ \d+ more/, text)
      assert_match(/↓ \d+ more/, text)
      assert_includes text, "▶ 100  STR #{var_name(40)}"
    end
  end

  def var_name(index) = "V#{("a".ord + ((index - 1) % 26)).chr}#{"x" * ((index - 1) / 26)}"

  def test_parse_size
    assert_equal [90, 20], ALI::TUI::Render.parse_size("90x20")
    assert_raises(ArgumentError) { ALI::TUI::Render.parse_size("90") }
  end
end
