# frozen_string_literal: true

module SetCalc
  module TUI
    # A grid of characters, each with a style. The screen draws on a canvas,
    # and the canvas makes the text for the terminal or for --render.
    #
    # The styles have one meaning each:
    #   :frame  gray: borders, labels and help
    #   :done   gray: a node that the command already processed
    #   :title  bold: a set name
    #   :focus  cyan: the node that the step looks at now
    #   :path   amber: the path from the root, a moved set, the narration mark
    #   :new    green: a node that the step added, a successful result
    #   :error  red: an error
    class Canvas
      SGR = {
        frame: "90", done: "90", title: "1", key: "1", focus: "1;36", path: "33",
        new: "1;32", error: "31"
      }.freeze

      attr_reader :width, :height

      def initialize(width, height)
        @width = width
        @height = height
        @chars = Array.new(height) { Array.new(width, " ") }
        @styles = Array.new(height) { Array.new(width) }
      end

      # Writes the text at column x of row y. The canvas clips the text at its
      # edges and at +limit+ (an exclusive right column).
      def put(x, y, text, style = nil, limit: @width)
        return if y.negative? || y >= @height

        right = [limit, @width].min
        text.each_char.with_index do |char, i|
          column = x + i
          next if column.negative?
          break if column >= right

          @chars[y][column] = char
          @styles[y][column] = style
        end
      end

      def char_at(x, y)
        @chars[y][x]
      end

      def style_at(x, y)
        @styles[y][x]
      end

      def row_text(y)
        @chars[y].join
      end

      def lines(color: false)
        Array.new(@height) { |y| color ? colored_row(y) : row_text(y) }
      end

      def to_s(color: false)
        lines(color: color).map(&:rstrip).join("\n")
      end

      private

      def colored_row(y)
        out = +""
        current = nil
        @width.times do |x|
          style = @styles[y][x]
          if style != current
            out << "\e[0m" if current
            out << "\e[#{SGR.fetch(style)}m" if style
            current = style
          end
          out << @chars[y][x]
        end
        out << "\e[0m" if current
        out
      end
    end
  end
end
