# frozen_string_literal: true

module ALI
  module TUI
    # The screen for a program with load errors: each error with its line,
    # its column and the source line.
    class ErrorScreen
      def initialize(file_name, errors, source)
        @file_name = file_name
        @errors = errors
        @lines = source.to_s.dup.force_encoding(Encoding::UTF_8).scrub.lines(chomp: true)
      end

      def draw(width, height)
        canvas = Canvas.new(width, height)
        noun = @errors.size == 1 ? "error" : "errors"
        canvas.text(1, 0, "ALI", bold: true).text(5, 0, "· #{@file_name}", :muted)
        canvas.text_right(width - 1, 0, "#{@errors.size} load #{noun}", :alert, bold: true)
        canvas.text(2, 2, "ALI cannot load the program. Correct these errors and start ALI again.", bold: true)
        draw_errors(canvas, width, height)
        canvas.text(2, height - 1, "q", bold: true).text(4, height - 1, "quit", :muted)
        canvas
      end

      private

      def draw_errors(canvas, width, height)
        row = 4
        last_row = height - 3
        @errors.each_with_index do |error, index|
          if row + 2 > last_row
            hidden = @errors.size - index
            canvas.text(2, row, "… and #{hidden} more #{hidden == 1 ? "error" : "errors"}. " \
                                "ALI prints all of them when you quit.", :muted)
            break
          end
          canvas.text(2, row, "●", :alert).text(4, row, fit(error.to_s, width - 6))
          source = error.line.positive? ? @lines[error.line - 1].to_s : ""
          canvas.text(6, row + 1, fit(source, width - 8), :muted) unless source.strip.empty?
          caret_column = 6 + error.column - 1
          canvas.text(caret_column, row + 2, "^", :alert) if error.column.positive? && caret_column < width
          row += 4
        end
      end

      def fit(text, width) = text.length > width ? "#{text[0, width - 1]}…" : text
    end

    # The first screen when ALI starts without a file: a list of programs.
    class Picker
      attr_reader :selection, :files

      def initialize(files)
        @files = files
        @selection = 0
      end

      def move(delta) = (@selection = (@selection + delta).clamp(0, [@files.size - 1, 0].max))
      def chosen = @files[@selection]

      def draw(width, height)
        canvas = Canvas.new(width, height)
        canvas.text(1, 0, "ALI", bold: true).text(5, 0, "· choose a program", :muted)
        left = [(width - 50) / 2, 2].max
        top = [(height - @files.size - 6) / 2, 2].max
        canvas.text(left, top, "Which program do you want to see?", bold: true)
        @files.each_with_index do |file, index|
          selected = index == @selection
          canvas.text(left, top + 2 + index, selected ? "▶" : " ", :target, bold: true)
          canvas.text(left + 2, top + 2 + index, File.basename(file), selected ? :plain : :muted, bold: selected)
        end
        canvas.text(left, top + 3 + @files.size, "No programs found in ./programs.", :muted) if @files.empty?
        canvas.text(2, height - 1, "⏎", bold: true).text(4, height - 1, "open", :muted)
        canvas.text(10, height - 1, "↑/↓", bold: true).text(14, height - 1, "choose", :muted)
        canvas.text(22, height - 1, "q", bold: true).text(24, height - 1, "quit", :muted)
        canvas
      end
    end
  end
end
