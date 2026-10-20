# frozen_string_literal: true

require "io/console"
require_relative "keys"
require_relative "player"
require_relative "screen"

module SetCalc
  module TUI
    # The interactive terminal app: raw mode, the alternate screen, and one
    # event loop. The loop waits for a key, a resize or the next animation
    # tick, with IO.select. It has no threads.
    class App
      ENTER = "\e[?1049h\e[?25l"
      LEAVE = "\e[0m\e[?25h\e[?1049l"

      def initialize(input: $stdin, output: $stdout, player: Player.new)
        @input = input
        @output = output
        @player = player
        @next_tick = nil
      end

      def run
        @resize_reader, resize_writer = IO.pipe
        previous = Signal.trap("WINCH") { resize_writer.write_nonblock(".", exception: false) }
        @output.write(ENTER)
        @input.raw { loop_until_quit }
        0
      ensure
        @output.write(LEAVE)
        @output.flush
        Signal.trap("WINCH", previous || "DEFAULT")
        @resize_reader&.close
        resize_writer&.close
      end

      private

      def loop_until_quit
        until @player.quit?
          draw
          ready = IO.select([@input, @resize_reader], nil, nil, timeout)
          ready ? handle_ready(ready.first) : tick
        end
      end

      def timeout
        return nil unless @player.playing?

        @next_tick ||= now + @player.interval
        [@next_tick - now, 0].max
      end

      def tick
        @player.tick
        @next_tick = @player.playing? ? now + @player.interval : nil
      end

      def handle_ready(ios)
        @resize_reader.read_nonblock(64, exception: false) if ios.include?(@resize_reader)
        return unless ios.include?(@input)

        bytes = @input.read_nonblock(1024, exception: false)
        return @player.press(:ctrl_d) if bytes.nil?
        return if bytes == :wait_readable

        was_playing = @player.playing?
        Keys.decode(bytes).each { |key| @player.press(key) }
        @next_tick = nil if was_playing != @player.playing? || !@player.playing?
      end

      def draw
        rows, columns = @output.winsize
        screen = Screen.new(@player, columns, rows)
        canvas = screen.draw
        x, y = screen.cursor
        cursor = screen.too_small? ? "" : "\e[#{y + 1};#{x + 1}H\e[?25h"
        @output.write("\e[?25l\e[H#{canvas.lines(color: true).join("\r\n")}#{cursor}")
        @output.flush
      end

      def now
        Process.clock_gettime(Process::CLOCK_MONOTONIC)
      end
    end
  end
end
