# frozen_string_literal: true

require_relative "node"

module SetCalc
  # A set of numbers, stored in an unbalanced binary search tree.
  #
  # The tree has no duplicates: two values a and b are the same element when
  # (a <=> b) is 0, so 2 and 2.0 are one element. The tree keeps the value
  # that it received first.
  #
  # The methods that walk the tree are iterative, so a long chain (for example
  # the values 1..1000 in order) cannot overflow the stack. Only #deep_copy is
  # recursive, because the assignment asks for a recursive deep copy.
  #
  # The methods that compare values yield events to an optional block. The
  # calculator uses the events to narrate each step:
  #
  #   [:compare, node_value, :left | :right | :equal]
  #   [:insert,  value, parent_value | nil, :left | :right | nil]
  class BST
    include Enumerable

    attr_reader :root, :size, :version

    def self.from(values)
      values.each_with_object(new) { |value, tree| tree.insert(value) }
    end

    def initialize
      @root = nil
      @size = 0
      @version = 0 # Changes on each mutation. Views use it to cache work.
    end

    def empty?
      @root.nil?
    end

    # Inserts the value and returns true, or returns false when the tree
    # already has an equal value.
    def insert(value, &observer)
      check_comparable(value)
      return attach(Node.new(value), nil, nil, &observer) if @root.nil?

      node = @root
      loop do
        side = direction(value, node.value)
        observer&.call(:compare, node.value, side)
        return false if side == :equal

        child = node.public_send(side)
        return attach(Node.new(value), node, side, &observer) if child.nil?

        node = child
      end
    end

    # Returns true when the tree has a value equal to the argument.
    def search(value, &observer)
      node = @root
      until node.nil?
        side = direction(value, node.value)
        observer&.call(:compare, node.value, side)
        return true if side == :equal

        node = node.public_send(side)
      end
      false
    end

    def include?(value)
      search(value)
    end

    alias member? include?

    # In-order traversal (left, node, right): the values in ascending order.
    def each
      return enum_for(:each) { @size } unless block_given?

      stack = []
      node = @root
      until node.nil? && stack.empty?
        while node
          stack.push(node)
          node = node.left
        end
        node = stack.pop
        yield node.value
        node = node.right
      end
      self
    end

    # Pre-order traversal (node, left, right). Inserting the values in this
    # order into an empty tree gives a tree with the same shape.
    def each_preorder
      return enum_for(:each_preorder) { @size } unless block_given?

      stack = [@root].compact
      until stack.empty?
        node = stack.pop
        yield node.value
        stack.push(node.right) if node.right
        stack.push(node.left) if node.left
      end
      self
    end

    # The number of levels: 0 for an empty tree, 1 for a single node.
    def height
      level = [@root].compact
      count = 0
      until level.empty?
        count += 1
        level = level.flat_map { |node| [node.left, node.right].compact }
      end
      count
    end

    # A new tree with new nodes and the same shape. The copy is recursive and
    # pre-order: copy the node, then its left subtree, then its right subtree.
    # After it attaches each node, it yields the value, the partial copy, the
    # parent value and the side.
    def deep_copy(&)
      copy = BST.new
      copy_subtree(@root, copy, nil, nil, &)
      copy
    end

    def ==(other)
      other.is_a?(BST) && to_a == other.to_a
    end

    alias eql? ==

    def hash
      to_a.hash
    end

    def to_s
      "{#{to_a.join(', ')}}"
    end

    alias inspect to_s

    protected

    def attach(node, parent, side)
      if parent.nil?
        @root = node
      else
        parent.public_send(:"#{side}=", node)
      end
      @size += 1
      @version += 1
      yield(:insert, node.value, parent&.value, side) if block_given?
      true
    end

    private

    def copy_subtree(source, copy, parent, side, &observer)
      return if source.nil?

      node = Node.new(source.value)
      copy.attach(node, parent, side)
      observer&.call(source.value, copy, parent&.value, side)
      copy_subtree(source.left, copy, node, :left, &observer)
      copy_subtree(source.right, copy, node, :right, &observer)
    end

    def direction(value, node_value)
      order = value <=> node_value
      return :equal if order.zero?

      order.negative? ? :left : :right
    end

    def check_comparable(value)
      return if value.is_a?(Numeric) && value.real? && !(value.respond_to?(:nan?) && value.nan?)

      raise ArgumentError, "a set holds real numbers only, not #{value.inspect}"
    end
  end
end
