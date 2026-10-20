# frozen_string_literal: true

module ALI
  module TUI
    # The --render developer mode: draws one frame of the visualizer as text
    # after a scripted sequence of commands. It needs no terminal, so the
    # tests and the README use it.
    module Render
      DEFAULT_SIZE = [120, 36].freeze
      # Characters in --keys that stand for named keys.
      KEY_NAMES = { "E" => "Enter", "U" => "Up", "D" => "Down" }.freeze

      module_function

      # Returns the frame as a String and the exit status.
      def frame(path, steps: 0, ops: 0, keys: "", size: DEFAULT_SIZE, color: false)
        width, height = size
        loaded = Assembler.assemble_file(path)
        name = File.basename(path)
        unless loaded.ok?
          source = File.file?(path) ? File.binread(path) : ""
          return [ErrorScreen.new(name, loaded.errors, source).draw(width, height).to_s(color:), 1]
        end

        controller = Controller.new(loaded.program)
        steps.times { controller.step_instruction }
        ops.times { controller.next_micro_op }
        # The app draws the screen after each key, and the Program pane follows
        # the selection only when it sees the selection move. So this mode also
        # draws after each key, to show the same frame as the app.
        screen = Screen.new(controller, name)
        screen.draw(width, height)
        keys.each_char do |char|
          Keys.handle(controller, KEY_NAMES.fetch(char, char))
          screen.draw(width, height)
        end
        [screen.draw(width, height).to_s(color:), 0]
      end

      # Parses "WxH".
      def parse_size(text)
        match = /\A(\d+)x(\d+)\z/.match(text.to_s)
        raise ArgumentError, "--size needs WxH, for example 120x36" unless match

        [match[1].to_i, match[2].to_i]
      end
    end
  end
end
