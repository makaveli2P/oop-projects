# frozen_string_literal: true

require_relative "../calculator"

module SetCalc
  module TUI
    # The state of the visualizer, without terminal code: the calculator, the
    # frames of the last command, the position in the frames, the animation
    # state and the command input line. The terminal app and --render both use
    # #press, so a test can drive the visualizer with keys.
    #
    # A command executes completely when you press Enter. The frames record
    # each step, so "next" and "back" only move in the recorded frames.
    class Player
      SPEEDS = [1, 2, 3, 4, 6, 8, 12, 20].freeze
      DEFAULT_SPEED = 4
      INPUT_LIMIT = 200
      SCROLL_STEP = 2
      # With an empty input line, these characters control the animation. No
      # command starts with them, so they never block typing.
      CONTROL_CHARS = { " " => :next, "+" => :faster, "=" => :faster, "-" => :slower }.freeze
      SYMBOL_ACTIONS = {
        right: :next, left: :back, tab: :toggle, home: :first, end: :last,
        up: :scroll_up, page_up: :scroll_up, down: :scroll_down, page_down: :scroll_down
      }.freeze

      attr_reader :calculator, :frames, :index, :input, :result, :speed, :scroll

      def initialize(calculator = Calculator.new)
        @calculator = calculator
        tracer = Tracer.new(calculator)
        tracer.finish("All three sets are empty. Type a command below and press Enter.")
        @frames = tracer.frames
        @index = 0
        @input = +""
        @result = nil
        @speed = DEFAULT_SPEED
        @playing = false
        @scroll = 0
        @scroll_range = nil
        @quit = false
        @started = false
      end

      def frame
        @frames[@index]
      end

      def playing?
        @playing
      end

      def quit?
        @quit
      end

      def started?
        @started
      end

      def at_end?
        @index == @frames.size - 1
      end

      def interval
        1.0 / @speed
      end

      def state
        return "Ready" unless @started
        return "Playing" if @playing

        at_end? ? "Done" : "Paused"
      end

      # The message line shows the result after the last frame. An error
      # shows at once.
      def message
        return nil if @result.nil?
        return @result if @result.error || at_end?

        nil
      end

      # One step of the animation. The terminal app calls it on a timer.
      def tick
        return unless @playing

        move_to(@index + 1)
        @playing = false if at_end?
      end

      def press(key)
        if key.is_a?(String)
          press_char(key)
        else
          press_symbol(key)
        end
        self
      end

      # The screen calls this after each draw with the scroll values that move
      # at least one tree view. The stored scroll stays in that range, so the
      # opposite key moves the view at once after the user scrolls too far.
      def limit_scroll(range)
        @scroll_range = range
        @scroll = @scroll.clamp(range)
      end

      # Types the line and presses Enter.
      def submit(line)
        @input = +line.to_s
        press(:enter)
      end

      private

      def press_char(char)
        if @input.empty? && CONTROL_CHARS.key?(char)
          control(CONTROL_CHARS[char])
        elsif char.match?(/\A[[:print:]]\z/) && char.ascii_only? && @input.length < INPUT_LIMIT
          @input << char
        end
      end

      def press_symbol(key)
        case key
        when :enter then execute_input
        when :backspace then @input.chop!
        when :escape then @input.empty? ? @quit = true : @input.clear
        when :ctrl_c then @quit = true
        when :ctrl_d then @quit = true if @input.empty?
        else control(SYMBOL_ACTIONS[key])
        end
      end

      def control(action)
        case action
        when :next then pause_and_move(@index + 1)
        when :back then pause_and_move(@index - 1)
        when :first then pause_and_move(0)
        when :last then pause_and_move(@frames.size - 1)
        when :toggle then toggle
        when :faster then change_speed(1)
        when :slower then change_speed(-1)
        when :scroll_up then scroll_by(-SCROLL_STEP)
        when :scroll_down then scroll_by(SCROLL_STEP)
        end
      end

      def toggle
        if @playing
          @playing = false
        elsif @frames.size > 1
          move_to(0) if at_end?
          @playing = true
        end
      end

      def pause_and_move(position)
        @playing = false
        move_to(position)
      end

      def move_to(position)
        position = position.clamp(0, @frames.size - 1)
        if position != @index
          @scroll = 0
          @scroll_range = nil
        end
        @index = position
      end

      def scroll_by(delta)
        @scroll += delta
        @scroll = @scroll.clamp(@scroll_range) if @scroll_range
      end

      def change_speed(delta)
        position = SPEEDS.index(@speed) + delta
        @speed = SPEEDS[position.clamp(0, SPEEDS.size - 1)]
      end

      def execute_input
        line = @input.strip
        return if line.empty?

        @input = +""
        tracer = Tracer.new(@calculator, command: line)
        result = @calculator.run(line, tracer: tracer)
        @result = result
        @quit = true if result.quit
        return if result.error || tracer.frames.empty?

        @started = true
        @frames = tracer.frames
        @index = 0
        @scroll = 0
        @scroll_range = nil
        @playing = @frames.size > 1
      end
    end
  end
end
