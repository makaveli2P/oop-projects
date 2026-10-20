# frozen_string_literal: true

require_relative "canvas"
require_relative "text"
require_relative "tree_view"

module SetCalc
  module TUI
    # Draws the complete screen of the visualizer for a Player:
    #
    #   header      the name, the speed and the state
    #   panels      one panel for each tree: the drawing, the set, the size
    #   narration   the sentence for this step, and "step k of n"
    #   message     the result of the last command
    #   input       the command line, the primary action
    #   keys        the key help
    class Screen
      MIN_WIDTH = 80
      MIN_HEIGHT = 22
      BOTTOM_ROWS = 7 # blank, narration (2), message, blank, input, keys
      LISTING_LINES = 2
      PLACEHOLDER = "type a command:  X 5,3,8,1,4  ·  a 7  ·  u  ·  i  ·  c  ·  r  ·  s  ·  l ->(x) { x * 2 }  ·  q"
      KEYS = [
        ["⏎", "run"], ["space/→", "next"], ["←", "back"], ["tab", "play/pause"], ["+/-", "speed"],
        ["end", "skip"], ["↑/↓", "scroll"], ["esc", "quit"]
      ].freeze

      attr_reader :canvas

      def initialize(player, width, height)
        @player = player
        @width = width
        @height = height
        @canvas = Canvas.new(width, height)
      end

      def self.render(player, width, height)
        new(player, width, height).draw
      end

      def too_small?
        @width < MIN_WIDTH || @height < MIN_HEIGHT
      end

      # The column and the row of the text cursor in the input line.
      def cursor
        return [0, 0] if too_small?

        [4 + [@player.input.length, @width - 6].min, @height - 2]
      end

      def draw
        too_small? ? draw_too_small : draw_screen
        @canvas
      end

      private

      def draw_screen
        frame = @player.frame
        draw_header
        draw_panels(frame, 1, @height - 1 - BOTTOM_ROWS)
        draw_narration(frame, @height - 6)
        draw_message(@height - 4)
        draw_input(@height - 2)
        draw_keys(@height - 1)
      end

      def draw_too_small
        lines = ["The terminal is too small.",
                 "Make it at least #{MIN_WIDTH} × #{MIN_HEIGHT}. Now it is #{@width} × #{@height}.",
                 "Press esc to quit."]
        top = [(@height - lines.size) / 2, 0].max
        lines.each_with_index do |line, i|
          line = Text.truncate(line, @width)
          @canvas.put([(@width - line.length) / 2, 0].max, top + i, line, i.zero? ? :title : :frame)
        end
      end

      def draw_header
        @canvas.put(1, 0, "Set calculator", :title)
        @canvas.put(15, 0, "  ·  three binary search trees", :frame)
        speed = "speed #{@player.speed} steps/s"
        state = @player.state
        right = @width - 1
        @canvas.put(right - state.length, 0, state, state_style(state))
        @canvas.put(right - state.length - 4 - speed.length, 0, speed, :frame)
      end

      def state_style(state)
        case state
        when "Playing" then :path
        when "Done" then :new
        else :frame
        end
      end

      def draw_panels(frame, top, height)
        panels = frame.panels
        count = panels.size
        base = @width / count
        extra = @width % count
        x = 0
        ranges = panels.each_with_index.map do |panel, i|
          width = base + (i < extra ? 1 : 0)
          view = draw_panel(panel, x, top, width, height)
          x += width
          view[:scroll_range]
        end
        ranges.compact!
        range = ranges.empty? ? 0..0 : ranges.map(&:begin).min..ranges.map(&:end).max
        @player.limit_scroll(range)
      end

      def draw_panel(panel, left, top, width, height)
        draw_border(left, top, width, height)
        draw_title(panel, left, top, width)
        inner_left = left + 2
        inner_width = width - 4
        bottom = top + height - 2 # the last inner row
        listing = listing_lines(panel, inner_width)
        @canvas.put(inner_left, bottom, stats(panel, inner_width), :frame)
        listing.reverse.each_with_index do |line, i|
          @canvas.put(inner_left, bottom - 1 - i, line, limit: inner_left + inner_width)
        end
        tree_top = top + 2
        tree_height = bottom - listing.size - 1 - tree_top
        TreeView.new(panel.snapshot, panel.marks, fit_width: inner_width)
                .draw(@canvas, left: inner_left, top: tree_top, width: inner_width, height: tree_height,
                               scroll: @player.scroll)
      end

      def draw_border(left, top, width, height)
        right = left + width - 1
        bottom = top + height - 1
        @canvas.put(left, top, "╭#{'─' * (width - 2)}╮", :frame)
        ((top + 1)...bottom).each do |y|
          @canvas.put(left, y, "│", :frame)
          @canvas.put(right, y, "│", :frame)
        end
        @canvas.put(left, bottom, "╰#{'─' * (width - 2)}╯", :frame)
      end

      def draw_title(panel, left, top, width)
        title = " #{panel.title} "
        @canvas.put(left + 2, top, title, :title)
        return unless panel.tag

        tag = Text.truncate(" #{panel.tag} ", width - title.length - 5)
        @canvas.put(left + 2 + title.length, top, tag, :path)
      end

      # The set in ascending order, on at most LISTING_LINES lines. A long set
      # ends with "… 12 more}".
      def listing_lines(panel, width)
        return [] if panel.empty?

        # A token wider than the panel ends with "…", so the listing never
        # shows a cut number that looks like a different number.
        tokens = panel.values.map { |value| Text.truncate(Text.clean(value.to_s), width - 2) }
        full = wrap_listing(tokens, width)
        return full if full

        first, used = fill_line(tokens, 0, width, "{")
        second, used = fill_line(tokens, used, width - " … 9999 more}".length, "")
        [first, "#{second} … #{tokens.size - used} more}"]
      end

      # Wraps "{a, b, c}" at spaces. Returns nil if it needs more lines.
      def wrap_listing(tokens, width)
        lines = []
        line = "{"
        tokens.each_with_index do |token, i|
          piece = "#{token}#{i == tokens.size - 1 ? '}' : ','}"
          start = line == "{" || line.empty?
          if !start && line.length + 1 + piece.length > width
            lines << line
            return nil if lines.size >= LISTING_LINES

            line = piece
          else
            line = start ? "#{line}#{piece}" : "#{line} #{piece}"
          end
        end
        lines << line
      end

      # Fills one line with "a, b, c," from tokens[start]. Returns the line and
      # the index of the first token that did not fit.
      def fill_line(tokens, start, width, prefix)
        line = prefix
        index = start
        while index < tokens.size
          piece = "#{tokens[index]},"
          joined = line == prefix ? "#{line}#{piece}" : "#{line} #{piece}"
          break if joined.length > width && line != prefix

          line = joined
          index += 1
        end
        [line, index]
      end

      def stats(panel, width)
        return Text.truncate("empty set", width) if panel.empty?

        elements = panel.size == 1 ? "1 element" : "#{panel.size} elements"
        choices = ["#{elements} · height #{panel.height}", "#{panel.size} · height #{panel.height}", elements]
        choices.find { |text| text.length <= width } || Text.truncate(panel.size.to_s, width)
      end

      def draw_narration(frame, row)
        position = position_text(frame)
        width = @width - 6 - (position.empty? ? 0 : position.length + 3)
        lines = Text.wrap(Text.clean(frame.narration), width)
        if lines.size > 2
          lines = lines.first(2)
          lines[1] = Text.truncate("#{lines[1]} …", width)
        end
        @canvas.put(2, row, "▸", :path)
        lines.each_with_index { |line, i| @canvas.put(4, row + i, line) }
        @canvas.put(@width - 2 - position.length, row, position, :frame)
      end

      def position_text(frame)
        return "" unless @player.started?

        "#{Text.truncate(Text.clean(frame.command.to_s), 14)} · step #{@player.index + 1} of #{@player.frames.size}"
      end

      def draw_message(row)
        result = @player.message
        return unless result

        if result.error
          @canvas.put(4, row, Text.truncate(Text.clean("error: #{result.summary}"), @width - 6), :error)
        elsif result.output.any?
          @canvas.put(4, row, Text.truncate(Text.clean(result.output.join("  ·  ")), @width - 6), :new)
        end
      end

      def draw_input(row)
        @canvas.put(2, row, "›", :path)
        input = @player.input
        if input.empty?
          @canvas.put(4, row, Text.truncate(PLACEHOLDER, @width - 6), :frame)
        else
          visible = input.length > @width - 7 ? input[-(@width - 7)..] : input
          @canvas.put(4, row, visible)
        end
      end

      def draw_keys(row)
        x = 2
        KEYS.each do |key, action|
          needed = key.length + 1 + action.length
          break if x + needed > @width - 1

          @canvas.put(x, row, key, :key)
          @canvas.put(x + key.length + 1, row, action, :frame)
          x += needed + 3
        end
      end
    end
  end
end
