# frozen_string_literal: true

module SetCalc
  # One node of a binary search tree. The value does not change after
  # construction; only the tree that owns the node changes the links.
  class Node
    attr_reader :value
    attr_accessor :left, :right

    def initialize(value)
      @value = value
      @left = nil
      @right = nil
    end

    def leaf?
      left.nil? && right.nil?
    end
  end
end
