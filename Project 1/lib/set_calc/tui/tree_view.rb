# frozen_string_literal: true

require_relative "text"

module SetCalc
  module TUI
    # Draws a tree snapshot on a canvas, inside a rectangle.
    #
    # Layout: each node takes its own columns in in-order sequence, so the
    # left subtree is always to the left of its parent and the right subtree to
    # the right. Row 2d holds the nodes at depth d. Row 2d + 1 holds the
    # branches to their children:
    #
    #           [5]
    #        ┌───┴───┐
    #       [3]     [8]
    #
    # When the tree is larger than the rectangle, the view scrolls so that the
    # focus node (or the root) is in the middle. Markers show the hidden part:
    # "▴ 2 levels above", "▾ 9 levels below", and ‹ or › at the end of a row
    # that continues out of the view.
    class TreeView
      GAP = 1
      ON_PATH = %i[path focus new].freeze
      PATH_PARENT = %i[path focus].freeze
      NODE_STYLE = { focus: :focus, path: :path, new: :new, done: :done }.freeze

      Item = Struct.new(:value, :label, :x, :center, :depth, :left, :right)

      attr_reader :width, :rows

      # +fit_width+: when the tree with "[5]" labels is wider than this, the
      # view uses bare labels ("5") to make the tree narrower.
      def initialize(snapshot, marks = {}, fit_width: nil)
        @marks = marks
        @bare = false
        @items = []
        @root = layout(snapshot)
        if fit_width && !@items.empty? && @items.last.x + @items.last.label.length > fit_width
          @bare = true
          @items = []
          @root = layout(snapshot)
        end
        @width = @items.empty? ? 0 : @items.last.x + @items.last.label.length
        @rows = @items.empty? ? 0 : (@items.map(&:depth).max * 2) + 1
        measure_rows
      end

      def empty?
        @items.empty?
      end

      # Draws the tree in the rectangle. +scroll+ moves the view down (positive)
      # or up (negative), in rows. Returns the visible part as a hash, for tests.
      def draw(canvas, left:, top:, width:, height:, scroll: 0)
        return draw_empty(canvas, left, top, width, height) if empty?

        @canvas = canvas
        vertical(top, height, scroll)
        horizontal(left, width)
        @items.each { |item| draw_item(item) }
        draw_markers(left, top, width, height)
        { x_offset: @ox, y_offset: @oy, clipped_x: @clip_x, clipped_y: @clip_y, scroll_range: @scroll_range }
      end

      private

      # In-order walk without recursion: a chain of 1000 nodes is fine.
      def layout(snapshot)
        item_of = {}.compare_by_identity
        stack = []
        node = snapshot
        depth = 0
        cursor = 0
        until node.nil? && stack.empty?
          while node
            stack.push([node, depth])
            node = node.left
            depth += 1
          end
          node, depth = stack.pop
          item = new_item(node.value, cursor, depth)
          item_of[node] = item
          cursor += item.label.length + GAP
          node = node.right
          depth += 1
        end
        item_of.each do |snap, item|
          item.left = item_of[snap.left]
          item.right = item_of[snap.right]
        end
        item_of[snapshot]
      end

      def new_item(value, x, depth)
        text = Text.clean(value.to_s)
        label = if @bare then text
                elsif @marks[value] == :focus then "(#{text})"
                else "[#{text}]"
                end
        item = Item.new(value, label, x, x + (label.length / 2), depth, nil, nil)
        @items << item
        item
      end

      # The first and the last used column of each virtual row.
      def measure_rows
        @row_min = Array.new(@rows) { Float::INFINITY }
        @row_max = Array.new(@rows) { -1 }
        @items.each do |item|
          extend_row(item.depth * 2, item.x, item.x + item.label.length - 1)
          left = item.left&.center || item.center
          right = item.right&.center || item.center
          extend_row((item.depth * 2) + 1, left, right) if item.left || item.right
        end
      end

      def extend_row(row, first, last)
        @row_min[row] = [@row_min[row], first].min
        @row_max[row] = [@row_max[row], last].max
      end

      def horizontal(left, width)
        @clip_x = @width > width
        @view_w = @clip_x ? width - 2 : width
        @view_left = @clip_x ? left + 1 : left
        @ox = if @clip_x
                (horizontal_anchor.center - (@view_w / 2)).clamp(0, @width - @view_w)
              else
                -((width - @width) / 2)
              end
      end

      def vertical(top, height, scroll)
        @clip_y = @rows > height
        @view_h = @clip_y ? height - 2 : height
        @view_top = @clip_y ? top + 1 : top
        unless @clip_y
          @oy = 0
          @scroll_range = nil # scrolling does not move this view
          return
        end

        base = (anchor.depth * 2) - (@view_h / 2)
        @oy = (base + scroll).clamp(0, @rows - @view_h)
        # The scroll values that move this view: past them the view is at the
        # first or the last row.
        @scroll_range = -base..(@rows - @view_h - base)
      end

      # The node that the view keeps in the middle: the focus node, else the
      # one new node, else the root. When many nodes are new (the last frame of
      # "X values"), the root is the anchor, so the view shows the top.
      def anchor
        @anchor ||= @items.find { |item| @marks[item.value] == :focus } || single_new || @root
      end

      def single_new
        added = @items.select { |item| @marks[item.value] == :new }
        added.first if added.size == 1
      end

      # The node that sets the horizontal offset. When the user scrolls the
      # anchor out of the view, it is the visible node nearest to the middle
      # row (and then to the anchor), so the view always shows a part of the
      # tree.
      def horizontal_anchor
        return anchor unless @clip_y

        first = @oy
        last = @oy + @view_h - 1
        return anchor if (first..last).cover?(anchor.depth * 2)

        middle = @oy + (@view_h / 2)
        visible = @items.select { |item| (first..last).cover?(item.depth * 2) }
        visible.min_by { |item| [((item.depth * 2) - middle).abs, (item.center - anchor.center).abs] } || anchor
      end

      # A node label is drawn only when it is completely in the view, so the
      # view never shows a part of a number.
      def draw_item(item)
        sx = item.x - @ox
        if sx >= 0 && sx + item.label.length <= @view_w
          plot(item.x, item.depth * 2, item.label, NODE_STYLE[@marks[item.value]])
        end
        draw_branches(item) if item.left || item.right
      end

      def draw_branches(item)
        row = (item.depth * 2) + 1
        center = item.center
        left_style = branch_style(item, item.left)
        right_style = branch_style(item, item.right)
        plot(item.left.center, row, "┌#{'─' * (center - item.left.center - 1)}", left_style) if item.left
        plot(center + 1, row, "#{'─' * (item.right.center - center - 1)}┐", right_style) if item.right
        joint = if item.left && item.right then "┴"
                elsif item.left then "┘"
                else "└"
                end
        plot(center, row, joint, [left_style, right_style].include?(:path) ? :path : :frame)
      end

      def branch_style(parent, child)
        return :frame if child.nil?

        on_path = ON_PATH.include?(@marks[child.value]) && PATH_PARENT.include?(@marks[parent.value])
        on_path ? :path : :frame
      end

      # Draws text at a virtual position, clipped to the view.
      def plot(vx, vy, text, style)
        return if vy < @oy || vy >= @oy + @view_h

        sx = @view_left + vx - @ox
        if sx < @view_left
          text = text[(@view_left - sx)..] || ""
          sx = @view_left
        end
        @canvas.put(sx, @view_top + vy - @oy, text, style, limit: @view_left + @view_w)
      end

      def draw_markers(left, top, width, height)
        draw_vertical_markers(left, top, width, height) if @clip_y
        return unless @clip_x

        @view_h.times do |i|
          vy = @oy + i
          next if vy >= @rows

          @canvas.put(left, @view_top + i, "‹", :frame) if @row_min[vy] < @ox
          @canvas.put(left + width - 1, @view_top + i, "›", :frame) if @row_max[vy] > @ox + @view_w - 1
        end
      end

      def draw_vertical_markers(left, top, width, height)
        above = (@oy + 1) / 2
        below = (@rows - (@oy + @view_h) + 1) / 2
        center_text(left, top, width, "▴ #{levels(above)} above") if above.positive?
        center_text(left, top + height - 1, width, "▾ #{levels(below)} below") if below.positive?
      end

      def levels(count)
        count == 1 ? "1 level" : "#{count} levels"
      end

      def center_text(left, row, width, text)
        text = Text.truncate(text, width)
        @canvas.put(left + ((width - text.length) / 2), row, text, :frame)
      end

      def draw_empty(canvas, left, top, width, height)
        canvas.put(left + ((width - 1) / 2), top + ((height - 1) / 2), "∅", :frame) if height.positive?
        { x_offset: 0, y_offset: 0, clipped_x: false, clipped_y: false, scroll_range: nil }
      end
    end
  end
end
