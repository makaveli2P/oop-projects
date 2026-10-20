# frozen_string_literal: true

module ALI
  module TUI
    # The state and the commands of the visualizer. It has no terminal code,
    # so the tests drive it directly.
    #
    # The controller shows one instruction at a time, one micro-operation at a
    # time. It executes the complete instruction on the Machine, keeps the
    # state before the instruction, and applies the first +cursor+
    # micro-operations of the trace to that state. The result is the view
    # state that the screen shows.
    class Controller
      HISTORY_LIMIT = 4096
      # The delay between two micro-operations at each speed level, in ms.
      DELAYS_MS = [1000, 600, 350, 200, 120, 70, 40, 20].freeze
      DEFAULT_SPEED = 3

      # One executed instruction: the state before it and its trace.
      Entry = Data.define(:before, :step)

      attr_reader :cursor, :speed_level, :breakpoints, :selection, :base, :message, :view, :previous

      def initialize(program, history_limit: HISTORY_LIMIT)
        @machine = Machine.new(program)
        @history = []
        @history_limit = [history_limit, 1].max
        @cursor = 0
        @running = false
        @run_started = false
        @speed_level = DEFAULT_SPEED
        @breakpoints = Set.new
        @selection = 0
        @base = :decimal
        @message = ""
        update_view
      end

      def program = @machine.program

      # ---- Commands ------------------------------------------------------

      # Shows the next micro-operation. At the end of an instruction, executes
      # the next instruction and shows its first micro-operation.
      def next_micro_op
        @message = ""
        if !at_boundary?
          @cursor += 1
        elsif instruction_started?
          @cursor = 1
        end
        update_view
      end

      # Completes the current instruction. If it is complete, executes the
      # next instruction and shows all of its micro-operations.
      def step_instruction
        @message = ""
        @cursor = micro_ops.size if !at_boundary? || instruction_started?
        update_view
      end

      # Goes back to the state after the previous instruction.
      def back
        @message = ""
        pause
        if @history.empty?
          @message = @machine.steps.zero? ? "This is the start of the program." : "The history holds no earlier steps."
          return
        end
        @machine.restore(@history.pop.before)
        @cursor = micro_ops.size
        update_view
      end

      # Puts the machine back in the state after load. Keeps the breakpoints.
      def reset
        pause
        @machine.reset
        @history.clear
        @cursor = 0
        @message = "The machine is reset."
        update_view
      end

      # Executes complete instructions until a halt, a breakpoint or +limit+
      # instructions. Returns :halted, :breakpoint, :step_limit or :already_halted.
      def run_to_end(limit: Machine::DEFAULT_STEP_LIMIT)
        @message = ""
        pause
        started_at_boundary = at_boundary?
        @cursor = micro_ops.size # Complete the current instruction.
        stop = run_loop(limit, started_at_boundary)
        update_view
        stop
      end

      # Starts or pauses the animated run.
      def toggle_run
        @message = ""
        return pause if running?
        return halt_message if at_boundary? && @machine.halted?

        @running = true
        # A run that starts at an instruction boundary does not stop at the
        # breakpoint of the next instruction. A run that starts in the middle
        # of an instruction stops at the next breakpoint.
        @run_started = at_boundary?
      end

      def pause = (@running = false)
      def running? = @running

      # One animation tick: the run shows one more micro-operation, or stops.
      # Returns false if the run is paused.
      def tick
        return false unless running?

        if at_boundary?
          return stop_run { halt_message } if @machine.halted?
          return stop_run { breakpoint_message } if !@run_started && breakpoint?(@machine.pc)

          instruction_started?
          @cursor = 1
          @run_started = false
        else
          @cursor += 1
        end
        stop_run { halt_message } if at_boundary? && @machine.halted?
        update_view
        true
      end

      def faster = (@speed_level = [@speed_level + 1, DELAYS_MS.size - 1].min)
      def slower = (@speed_level = [@speed_level - 1, 0].max)
      def tick_interval = DELAYS_MS[@speed_level] / 1000.0

      # The rate in micro-operations each second, as short text.
      def speed_text
        rate = 1000.0 / DELAYS_MS[@speed_level]
        rate == rate.round ? rate.round.to_s : format("%.1f", rate)
      end

      def toggle_breakpoint(address = selection)
        return unless address.between?(0, program.size - 1)

        @breakpoints.delete?(address) || @breakpoints.add(address)
      end

      def breakpoint?(address) = @breakpoints.include?(address)

      def move_selection(delta)
        @selection = (@selection + delta).clamp(0, program.size - 1)
      end

      def select(address)
        @selection = address if address.between?(0, program.size - 1)
      end

      def toggle_base = (@base = @base == :decimal ? :hex : :decimal)

      # ---- Observation ---------------------------------------------------

      # True when an instruction is on the screen (complete or in part).
      def instruction? = !@history.empty?

      # The address of the instruction on the screen, or nil.
      def instruction_address = @history.last&.step&.address

      # The micro-operations of the instruction on the screen.
      def micro_ops = @history.last&.step&.micro_ops || []

      # The last visible micro-operation, or nil.
      def active_micro_op = @cursor.zero? ? nil : micro_ops[@cursor - 1]

      def instruction_complete? = instruction? && at_boundary?

      # The 1-based number of the instruction on the screen. 0 if none.
      def step_number = @history.empty? ? 0 : @history.last.before.steps + 1

      def halted? = view.halted?
      def halt_reason = view.halt_reason
      def history_size = @history.size

      private

      def at_boundary? = @history.empty? || @cursor == micro_ops.size

      # Executes the next instruction and shows none of its micro-operations.
      # Returns false, and sets the halt message, if the machine is halted.
      def instruction_started?
        if @machine.halted?
          halt_message
          return false
        end

        before = @machine.state
        @history << Entry.new(before:, step: @machine.step)
        @history.shift if @history.size > @history_limit
        @cursor = 0
        true
      end

      def run_loop(limit, started_at_boundary)
        stop = run_until_stop(limit, started_at_boundary)
        case stop
        when :breakpoint then breakpoint_message
        when :step_limit then limit_message(limit)
        else halt_message
        end
        stop
      end

      def run_until_stop(limit, started_at_boundary)
        return :already_halted if @machine.halted?

        count = 0
        until @machine.halted?
          at_start = started_at_boundary && count.zero?
          return :breakpoint if !at_start && breakpoint?(@machine.pc)
          return :step_limit if count == limit

          instruction_started?
          @cursor = micro_ops.size
          count += 1
        end
        :halted
      end

      # Pauses the run and sets the message that the block makes. A stop is
      # a tick too, so it returns true.
      def stop_run # rubocop:disable Naming/PredicateMethod
        pause
        yield
        true
      end

      def halt_message
        reason = @machine.halt_reason
        @message = "The machine is halted (#{MicroOp.halt_text(reason)}). Press R to reset." if reason
        @message
      end

      def breakpoint_message = (@message = "Stopped at the breakpoint at address #{@machine.pc}.")

      def limit_message(limit)
        @message = "ALI ran #{limit} instructions and the machine did not halt. Press g to run #{limit} more."
      end

      def update_view
        entry = @history.last
        if entry.nil?
          @view = @previous = @machine.state
        else
          @previous = entry.before
          @view = if at_boundary?
                    @machine.state
                  else
                    entry.step.micro_ops.first(@cursor).reduce(entry.before) { |state, op| op.apply(state) }
                  end
        end
      end
    end
  end
end
