# frozen_string_literal: true

require "test_helper"

class TestTracer < Minitest::Test
  def narrations(frames)
    frames.map(&:narration)
  end

  def test_insert_narrates_the_path
    calc = calculator_after("X 5,3,8")
    _, frames = frames_for(calc, "a 7")

    assert_equal ["a 7: insert 7 into X. Start at the root.", "7 > 5, go right.", "7 < 8, go left.",
                  "Put 7 as the left child of 8.", "X = {3, 5, 7, 8}"], narrations(frames)
    assert_equal({ 5 => :focus }, frames[1].panel(:X).marks)
    assert_equal({ 5 => :path, 8 => :focus }, frames[2].panel(:X).marks)
    assert_equal({ 5 => :path, 8 => :path, 7 => :new }, frames[3].panel(:X).marks)
  end

  def test_frames_keep_earlier_snapshots
    calc = calculator_after("X 5")
    _, frames = frames_for(calc, "a 7")

    assert_nil frames.first.panel(:X).snapshot.right
    assert_equal 7, frames.last.panel(:X).snapshot.right.value
  end

  def test_unchanged_trees_share_a_snapshot
    calc = calculator_after("X 5,3,8", "Y 1")
    _, frames = frames_for(calc, "a 7")

    assert_same frames[0].panel(:Y).snapshot, frames[-1].panel(:Y).snapshot
    assert_same frames[0].panel(:X).snapshot, frames[2].panel(:X).snapshot
  end

  def test_duplicate_insert
    _, frames = frames_for(calculator_after("X 5"), "a 5")

    assert_includes narrations(frames), "5 = 5: 5 is already in X. Skip it."
  end

  def test_rotate_tags
    _, frames = frames_for(calculator_after("X 1", "Y 2", "Z 3"), "r")

    assert_equal 2, frames.size
    assert_equal ["▸ to Y", "▸ to Z", "▸ to X"], frames.first.panels.map(&:tag)
    assert_equal ["◂ from Z", "◂ from X", "◂ from Y"], frames.last.panels.map(&:tag)
    assert_equal [[3], [1], [2]], frames.last.panels.map(&:values)
  end

  def test_union_visits_y_in_preorder
    _, frames = frames_for(calculator_after("X 5", "Y 8,2,9"), "u")
    visits = narrations(frames).grep(/\AVisit/)

    assert_equal ["Visit 8 in Y. Insert 8 into X.", "Visit 2 in Y. Insert 2 into X.",
                  "Visit 9 in Y. Insert 9 into X."], visits
    assert_equal "◂ X ∪ Y", frames.last.panel(:X).tag
  end

  def test_intersection_shows_a_result_panel_until_the_end
    _, frames = frames_for(calculator_after("X 5,3", "Y 3"), "i")

    assert_equal(%w[X Y Z X∩Y], frames.first.panels.map { |p| p.title.delete(" ") })
    assert_includes narrations(frames), "3 = 3: found 3 in Y. Insert 3 into X ∩ Y."
    assert_includes narrations(frames), "5 is not in Y: the search reached an empty place. Skip 5."
    assert_equal 3, frames.last.panels.size
    assert_equal [3], frames.last.panel(:X).values
  end

  def test_copy_grows_y_node_by_node
    _, frames = frames_for(calculator_after("X 5,3,8"), "c")
    sizes = frames.map { |frame| frame.panel(:Y).size }

    assert_equal [0, 1, 2, 3, 3], sizes
    assert_equal "Copy 3 into Y as the left child of 5.", frames[2].narration
  end

  def test_lambda_steps
    _, frames = frames_for(calculator_after("X 2,1"), "l ->(x) { x * 10 }")

    assert_equal ["f(1) = 10", "f(2) = 20"], narrations(frames)[1..2]
  end

  def test_errors_record_no_frames
    _, frames = frames_for(SetCalc::Calculator.new, "X 1,zz")

    assert_empty frames
  end

  def test_budget_keeps_the_final_frame
    calc = SetCalc::Calculator.new
    tracer = SetCalc::Tracer.new(calc, budget: 10)
    calc.run("X #{(1..50).to_a.join(',')}", tracer: tracer)

    assert_equal 11, tracer.frames.size
    assert_match(/skips the later steps\.\)\z/, tracer.frames.last.narration)
    assert_equal 50, tracer.frames.last.panel(:X).size
  end
end
