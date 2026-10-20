# frozen_string_literal: true

require "test_helper"

class TestTreeView < Minitest::Test
  def draw(values, width:, height:, marks: {}, scroll: 0)
    snapshot = SetCalc::Snapshot.of(tree(*values).root)
    canvas = SetCalc::TUI::Canvas.new(width, height)
    info = SetCalc::TUI::TreeView.new(snapshot, marks, fit_width: width)
                                 .draw(canvas, left: 0, top: 0, width: width, height: height, scroll: scroll)
    [canvas.lines.map(&:rstrip), info]
  end

  def test_balanced_tree
    lines, = draw([5, 3, 8, 1, 4, 7, 9], width: 31, height: 5)

    assert_equal(["            [5]",
                  "     ┌───────┴───────┐",
                  "    [3]             [8]",
                  " ┌───┴───┐       ┌───┴───┐",
                  "[1]     [4]     [7]     [9]"], lines.map { |l| l.delete_prefix("  ") })
  end

  def test_single_children
    lines, = draw([5, 3, 4], width: 12, height: 5)

    assert_equal ["        [5]", " ┌───────┘", "[3]", " └───┐", "    [4]"], lines
  end

  def test_focus_label
    lines, = draw([5, 3], width: 12, height: 3, marks: { 3 => :focus })

    assert_includes lines.join, "(3)"
  end

  def test_compacts_to_bare_labels
    lines, info = draw([5, 3, 8, 1, 4, 7, 9], width: 16, height: 5)

    refute info[:clipped_x]
    refute_includes lines.join, "["
    assert_equal "1   4   7   9", lines.last.strip
  end

  def test_long_chain_scrolls_to_the_focus
    lines, info = draw((1..20).to_a, width: 20, height: 10, marks: { 15 => :focus })

    assert info[:clipped_y]
    assert info[:clipped_x]
    assert_includes lines.join("\n"), "15"
    assert_match(/▴ \d+ levels above/, lines.first)
    assert_match(/▾ \d+ levels? below/, lines.last)
  end

  def test_never_draws_part_of_a_label
    (1..30).each do |focus|
      lines, = draw((100..130).to_a, width: 24, height: 12, marks: { 100 + focus => :focus })
      numbers = lines[1..-2].join(" ").scan(/\d+/)

      assert(numbers.all? { |n| n.length == 3 }, "focus #{focus}: #{numbers.inspect}")
    end
  end

  def balanced(low, high)
    return [] if low > high

    middle = (low + high) / 2
    [middle, *balanced(low, middle - 1), *balanced(middle + 1, high)]
  end

  def test_markers_for_hidden_columns
    lines, info = draw(balanced(1, 31), width: 20, height: 9)
    body = lines

    assert info[:clipped_x]
    refute info[:clipped_y]

    assert(body.any? { |l| l.start_with?("‹") })
    assert(body.any? { |l| l.end_with?("›") })
  end

  def test_scroll_moves_the_view
    first, = draw((1..30).to_a, width: 40, height: 10)
    down, = draw((1..30).to_a, width: 40, height: 10, scroll: 6)

    refute_match(/above/, first.first)
    assert_match(/above/, down.first)
  end

  def test_empty_tree
    canvas = SetCalc::TUI::Canvas.new(9, 3)
    SetCalc::TUI::TreeView.new(nil).draw(canvas, left: 0, top: 0, width: 9, height: 3)

    assert_equal "    ∅", canvas.row_text(1).rstrip
  end

  def test_path_branches_are_amber
    snapshot = SetCalc::Snapshot.of(tree(5, 8).root)
    canvas = SetCalc::TUI::Canvas.new(12, 3)
    SetCalc::TUI::TreeView.new(snapshot, { 5 => :path, 8 => :new }).draw(canvas, left: 0, top: 0, width: 12, height: 3)
    row = canvas.row_text(1)
    column = row.index("┐")

    assert_equal :path, canvas.style_at(column, 1)
    assert_equal :new, canvas.style_at(canvas.row_text(2).index("8"), 2)
  end

  def test_many_new_nodes_anchor_on_the_root
    values = [50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43, 56, 68, 81, 93, 3, 9]
    marks = values.to_h { |value| [value, :new] }
    lines, info = draw(values, width: 20, height: 5, marks: marks)

    assert_equal 0, info[:y_offset]
    assert_includes lines.join, "50"
  end

  def test_one_new_node_is_the_anchor
    values = (1..20).to_a
    _, info = draw(values, width: 20, height: 7, marks: { 20 => :new })

    assert_operator info[:y_offset], :>, 0
  end

  def test_scrolled_view_always_shows_a_node
    values = (1..20).to_a
    _, info = draw(values, width: 20, height: 9)
    range = info[:scroll_range]

    range.step(2).each do |scroll|
      lines, = draw(values, width: 20, height: 9, scroll: scroll)
      body = lines[1..-2].join

      assert_match(/\d/, body, "scroll #{scroll} shows no node")
    end
  end

  def test_scroll_range_reaches_the_first_and_last_rows
    values = (1..20).to_a
    _, info = draw(values, width: 20, height: 9)
    range = info[:scroll_range]
    _, top = draw(values, width: 20, height: 9, scroll: range.begin)
    _, bottom = draw(values, width: 20, height: 9, scroll: range.end)

    assert_equal 0, top[:y_offset]
    assert_equal 39 - 7, bottom[:y_offset]
    assert_nil draw([5, 3, 8], width: 20, height: 9)[1][:scroll_range]
  end
end
