# frozen_string_literal: true

require "io/console"

module ALI
  module TUI
    # The terminal: raw input, the alternate screen, the mouse and the window size.
    #
    # The app has one thread. Terminal#read_keys waits for input with
    # IO.select. The timeout of the wait is the animation clock, so the app
    # needs no ticker thread. A resize signal writes to a pipe, and the pipe
    # wakes the wait.
    class Terminal
      ENTER = "\e[?1049h\e[?25l\e[?1000h\e[?1006h"
      LEAVE = "\e[?1006l\e[?1000l\e[?25h\e[?1049l"

      # A mouse click: the 0-based column and row.
      Click = Data.define(:x, :y)

      def self.open(input: $stdin, output: $stdout)
        terminal = new(input, output)
        terminal.start
        yield terminal
      ensure
        terminal&.stop
      end

      def initialize(input, output)
        @in = input
        @out = output
        @wake_reader, @wake_writer = IO.pipe
        @last_frame = nil
      end

      def start
        @in.raw!
        @old_winch = Signal.trap("WINCH") { @wake_writer.write_nonblock(".", exception: false) }
        @out.write(ENTER)
        @out.flush
      end

      def stop
        @out.write("\e[0m#{LEAVE}")
        @out.flush
        @in.cooked!
        Signal.trap("WINCH", @old_winch || "DEFAULT")
      end

      # The size as [columns, rows].
      def size
        rows, columns = @out.winsize
        [columns, rows]
      rescue SystemCallError
        Render::DEFAULT_SIZE
      end

      # Writes a frame. It writes only the rows that changed since the last frame.
      def draw(canvas)
        lines = canvas.lines(color: true)
        @last_frame = nil if @last_frame&.size != lines.size
        buffer = +""
        lines.each_with_index do |line, row|
          next if @last_frame && @last_frame[row] == line

          buffer << "\e[#{row + 1};1H" << line
        end
        @last_frame = lines
        @out.write(buffer)
        @out.flush
      end

      # Forgets the last frame, so that the next draw writes all rows.
      def invalidate = (@last_frame = nil)

      # Waits up to +timeout+ seconds (nil: no limit) and returns the keys.
      # Each key is a name ("Up", "Enter", "q", ...) or a Click. Returns
      # [:resize] after a resize.
      def read_keys(timeout)
        ready, = IO.select([@in, @wake_reader], nil, nil, timeout)
        return [] unless ready
        return drain_wake if ready.include?(@wake_reader)

        decode(@in.read_nonblock(256, exception: false).to_s)
      end

      SEQUENCES = { "\e[A" => "Up", "\e[B" => "Down", "\eOA" => "Up", "\eOB" => "Down" }.freeze
      MOUSE = /\A\e\[<(\d+);(\d+);(\d+)([Mm])/

      private

      def drain_wake
        @wake_reader.read_nonblock(64, exception: false)
        invalidate
        [:resize]
      end

      def decode(bytes)
        keys = []
        until bytes.empty?
          key, length = decode_one(bytes)
          keys << key if key
          bytes = bytes[length..]
        end
        keys
      end

      def decode_one(bytes)
        if (match = MOUSE.match(bytes))
          button, x, y, kind = match.captures
          click = Click.new(x: x.to_i - 1, y: y.to_i - 1) if kind == "M" && button == "0"
          return [click, match[0].length]
        end
        SEQUENCES.each { |sequence, name| return [name, sequence.length] if bytes.start_with?(sequence) }
        return [nil, Regexp.last_match(0).length] if bytes =~ /\A\e\[[0-9;]*[A-Za-z~]/
        return ["Escape", 1] if bytes.start_with?("\e")
        return ["Enter", 1] if ["\r", "\n"].include?(bytes[0])
        return ["q", 1] if bytes[0] == "\u0003" # Ctrl-C

        [bytes[0], 1]
      end
    end
  end
end
