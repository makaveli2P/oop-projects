# frozen_string_literal: true

module ALI
  module TUI
    # Maps a key to a controller command. The interactive app and the
    # --render mode use the same table.
    module Keys
      # Key name -> the command. A command is a method of Controller, or :quit.
      BINDINGS = {
        " " => :next, "n" => :next,
        "s" => :step,
        "b" => :back,
        "r" => :toggle_run,
        "+" => :faster, "=" => :faster,
        "-" => :slower, "_" => :slower,
        "R" => :reset,
        "g" => :run_to_end,
        "x" => :toggle_base,
        "Enter" => :toggle_breakpoint,
        "Up" => :up, "k" => :up,
        "Down" => :down, "j" => :down,
        "q" => :quit, "Escape" => :quit
      }.freeze

      module_function

      # Applies one key. Returns :quit, :handled or :ignored.
      def handle(controller, key)
        command = BINDINGS[key]
        return :ignored unless command

        controller.pause if %i[next step quit].include?(command)
        return :quit if command == :quit

        apply(controller, command)
        :handled
      end

      def apply(controller, command)
        case command
        when :next then controller.next_micro_op
        when :step then controller.step_instruction
        when :up then controller.move_selection(-1)
        when :down then controller.move_selection(1)
        else controller.public_send(command)
        end
      end
    end
  end
end
