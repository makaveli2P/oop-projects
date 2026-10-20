# frozen_string_literal: true

module SetCalc
  module TUI
    # Helpers for text on a character grid. Each character that the screen
    # shows must take exactly one column, so #clean replaces control characters
    # and wide characters (for example from the result of a lambda).
    module Text
      # Single-width characters outside ASCII that the screen uses.
      SAFE = /[·…‹›←-⇿∅∩∪≤≥─-╿■-◿]/
      ELLIPSIS = "…"

      module_function

      def clean(text)
        text.to_s.each_char.map do |char|
          if char.match?(/[[:cntrl:]]/)
            " "
          elsif char.ord < 128 || SAFE.match?(char)
            char
          else
            format("\\u{%X}", char.ord)
          end
        end.join
      end

      # Cuts the text to +width+ columns. A cut text ends with an ellipsis.
      def truncate(text, width)
        return "" if width <= 0
        return text if text.length <= width

        "#{text[0, width - 1]}#{ELLIPSIS}"
      end

      # Splits the text into lines of at most +width+ columns, at spaces when
      # possible.
      def wrap(text, width)
        return [text] if width <= 0 || text.length <= width

        lines = []
        rest = text
        while rest.length > width
          cut = rest.rindex(" ", width) || width
          cut = width if cut.zero?
          lines << rest[0, cut].rstrip
          rest = rest[cut..].lstrip
        end
        lines << rest unless rest.empty?
        lines
      end
    end
  end
end
