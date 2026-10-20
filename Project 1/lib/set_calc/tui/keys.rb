# frozen_string_literal: true

module SetCalc
  module TUI
    # Turns terminal bytes into keys. A key is a one-character String for a
    # printable character, or a Symbol: :enter, :tab, :backspace, :escape,
    # :left, :right, :up, :down, :home, :end, :page_up, :page_down, :ctrl_c,
    # :ctrl_d. Unknown sequences become no key.
    module Keys
      SEQUENCES = {
        "\e[A" => :up, "\e[B" => :down, "\e[C" => :right, "\e[D" => :left,
        "\eOA" => :up, "\eOB" => :down, "\eOC" => :right, "\eOD" => :left,
        "\e[H" => :home, "\e[F" => :end, "\eOH" => :home, "\eOF" => :end,
        "\e[1~" => :home, "\e[4~" => :end, "\e[7~" => :home, "\e[8~" => :end,
        "\e[5~" => :page_up, "\e[6~" => :page_down, "\e[Z" => :tab
      }.freeze
      CONTROL = {
        "\r" => :enter, "\n" => :enter, "\t" => :tab, "\x7F" => :backspace, "\b" => :backspace,
        "\x03" => :ctrl_c, "\x04" => :ctrl_d
      }.freeze
      # Names for --render --keys, for example "a 7<enter><right>".
      NAMES = {
        "enter" => :enter, "tab" => :tab, "bs" => :backspace, "backspace" => :backspace,
        "esc" => :escape, "left" => :left, "right" => :right, "up" => :up, "down" => :down,
        "home" => :home, "end" => :end, "pgup" => :page_up, "pgdn" => :page_down, "space" => " "
      }.freeze

      module_function

      def decode(bytes)
        keys = []
        text = bytes.dup.force_encoding(Encoding::UTF_8).scrub("")
        until text.empty?
          key, length = next_key(text)
          keys << key if key
          text = text[length..]
        end
        keys
      end

      def next_key(text)
        return escape_key(text) if text.start_with?("\e")

        char = text[0]
        return [CONTROL[char], 1] if CONTROL.key?(char)
        return [nil, 1] if char.match?(/[[:cntrl:]]/)

        [char, 1]
      end

      def escape_key(text)
        return [:escape, 1] if text.length == 1

        match = text.match(/\A\e(?:\[[0-9;]*[A-Za-z~]|O[A-Za-z])/)
        return [:escape, 1] unless match

        [SEQUENCES[match[0]], match[0].length]
      end

      # Parses a key script: plain characters, and names in angle brackets.
      def script(text)
        text.scan(/<([a-z]+)>|(.)/m).map do |name, char|
          next char unless name

          NAMES.fetch(name) { raise ArgumentError, "unknown key name <#{name}>" }
        end
      end
    end
  end
end
