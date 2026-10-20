# frozen_string_literal: true

module ALI
  module TUI
    # The colors. Each ink has one meaning:
    #   :muted  frames and labels       :source  the unit that gives a value
    #   :target the unit that gets it,  :ok      a normal halt
    #           the path, a change      :alert   breakpoints and faults
    module Theme
      INKS = {
        plain: nil,
        muted: "38;5;244",
        source: "38;5;80",
        target: "38;5;214",
        ok: "38;5;78",
        alert: "38;5;203"
      }.freeze
      SELECTED_BACKGROUND = "48;5;236"

      module_function

      def sgr(style)
        codes = [INKS.fetch(style.ink)]
        codes << "1" if style.bold
        codes << SELECTED_BACKGROUND if style.selected
        codes.compact!
        codes.empty? ? "" : "\e[#{codes.join(";")}m"
      end
    end

    # The style of one cell.
    Style = Data.define(:ink, :bold, :selected) do
      def initialize(ink: :plain, bold: false, selected: false) = super
    end

    # A grid of cells, one character for each cell. The views draw on a
    # canvas, and the canvas makes the text for the terminal. All glyphs on
    # the screen take one column.
    class Canvas
      PLAIN = Style.new
      Cell = Struct.new(:char, :style)

      attr_reader :width, :height

      def initialize(width, height)
        @width = width
        @height = height
        @rows = Array.new(height) { Array.new(width) { Cell.new(" ", PLAIN) } }
      end

      # Writes +text+ from column +x+. The canvas clips the text at its edge.
      def text(x, y, text, ink = :plain, bold: false, selected: false)
        return self unless y.between?(0, height - 1)

        style = Style.new(ink:, bold:, selected:)
        text.to_s.each_char.with_index(x) do |char, column|
          @rows[y][column] = Cell.new(char, style) if column.between?(0, width - 1)
        end
        self
      end

      # Writes +text+ so that it ends at column +right+ (exclusive).
      def text_right(right, y, text, ink = :plain, **)
        self.text(right - text.length, y, text, ink, **)
      end

      # Sets the style of the cells from +x+ to +x+ + +length+ - 1 and keeps the characters.
      def restyle(x, y, length, **style)
        return self unless y.between?(0, height - 1)

        (x...(x + length)).each do |column|
          cell = @rows[y][column] if column.between?(0, width - 1)
          cell.style = cell.style.with(**style) if cell
        end
        self
      end

      def char_at(x, y) = @rows[y][x].char
      def style_at(x, y) = @rows[y][x].style

      # Draws a rounded frame with a title in the top edge.
      def frame(x, y, w, h, title: nil, ink: :muted, corners: "╭╮╰╯")
        top_left, top_right, bottom_left, bottom_right = corners.chars
        text(x, y, top_left + ("─" * (w - 2)) + top_right, ink)
        text(x, y + h - 1, bottom_left + ("─" * (w - 2)) + bottom_right, ink)
        (y + 1).upto(y + h - 2) do |row|
          text(x, row, "│", ink)
          text(x + w - 1, row, "│", ink)
        end
        text(x + 2, y, " #{title} ", :plain, bold: true) if title
        self
      end

      # Returns one String for each row. With +color+, the rows hold ANSI SGR codes.
      def lines(color: false)
        @rows.map do |row|
          next row.map(&:char).join unless color

          row.chunk_while { |left, right| left.style == right.style }.map do |run|
            sgr = Theme.sgr(run.first.style)
            sgr.empty? ? run.map(&:char).join : "#{sgr}#{run.map(&:char).join}\e[0m"
          end.join
        end
      end

      def to_s(color: false) = lines(color:).join("\n")
    end
  end
end
