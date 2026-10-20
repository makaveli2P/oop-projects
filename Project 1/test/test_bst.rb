# frozen_string_literal: true

require "test_helper"

class TestBST < Minitest::Test
  def test_empty_tree
    t = SetCalc::BST.new

    assert_empty t
    assert_equal 0, t.size
    assert_equal 0, t.height
    assert_equal [], t.to_a
    assert_equal "{}", t.to_s
  end

  def test_insert_keeps_order_and_rejects_duplicates
    t = SetCalc::BST.new

    assert t.insert(5)
    assert t.insert(3)
    refute t.insert(5)
    assert t.insert(8)
    assert_equal [3, 5, 8], t.to_a
    assert_equal 3, t.size
  end

  def test_shape_follows_insertion_order
    t = tree(5, 3, 8, 1, 4)

    assert_equal 5, t.root.value
    assert_equal 3, t.root.left.value
    assert_equal 8, t.root.right.value
    assert_equal 1, t.root.left.left.value
    assert_equal 4, t.root.left.right.value
    assert_equal 3, t.height
  end

  def test_integer_and_float_with_same_value_are_one_element
    t = tree(2, 2.0, 1.5)

    assert_equal [1.5, 2], t.to_a
    assert_includes t, 2.0
  end

  def test_rejects_values_that_are_not_real_numbers
    t = SetCalc::BST.new

    assert_raises(ArgumentError) { t.insert("5") }
    assert_raises(ArgumentError) { t.insert(Float::NAN) }
    assert_raises(ArgumentError) { t.insert(Complex(1, 2)) }
  end

  def test_insert_events_show_the_path
    t = tree(5, 3, 8)
    events = []
    t.insert(7) { |*event| events << event }

    assert_equal [[:compare, 5, :right], [:compare, 8, :left], [:insert, 7, 8, :left]], events
  end

  def test_duplicate_insert_event_is_equal
    events = []

    refute tree(5, 3).insert(3) { |*event| events << event }
    assert_equal [[:compare, 5, :left], [:compare, 3, :equal]], events
  end

  def test_search
    t = tree(5, 3, 8)
    events = []

    assert t.search(8)
    refute t.search(4) { |*event| events << event }
    assert_equal [[:compare, 5, :left], [:compare, 3, :right]], events
  end

  def test_preorder
    assert_equal [5, 3, 1, 4, 8, 7], tree(5, 3, 8, 1, 4, 7).each_preorder.to_a
  end

  def test_preorder_rebuilds_the_same_shape
    t = tree(10, 4, 15, 2, 6, 12, 20, 5)
    rebuilt = SetCalc::BST.from(t.each_preorder.to_a)

    assert_equal t.each_preorder.to_a, rebuilt.each_preorder.to_a
  end

  def test_long_chain_has_no_stack_overflow
    t = SetCalc::BST.from(1..3000)

    assert_equal 3000, t.height
    assert_equal (1..3000).to_a, t.to_a
    assert t.search(3000)
  end

  def test_deep_copy_has_new_nodes_and_the_same_shape
    t = tree(5, 3, 8, 1, 4)
    copy = t.deep_copy

    assert_equal t, copy
    assert_equal t.each_preorder.to_a, copy.each_preorder.to_a
    refute_same t.root, copy.root
    refute_same t.root.left, copy.root.left
    copy.insert(100)

    refute_includes t, 100
  end

  def test_deep_copy_yields_each_node_in_preorder
    seen = []
    tree(5, 3, 8, 1).deep_copy { |value, partial, parent, side| seen << [value, partial.size, parent, side] }

    assert_equal [[5, 1, nil, nil], [3, 2, 5, :left], [1, 3, 3, :left], [8, 4, 5, :right]], seen
  end

  def test_deep_copy_of_empty_tree
    assert_empty SetCalc::BST.new.deep_copy
  end

  def test_version_changes_only_on_mutation
    t = tree(1)
    before = t.version
    t.insert(1)

    assert_equal before, t.version
    t.insert(2)

    refute_equal before, t.version
  end
end
