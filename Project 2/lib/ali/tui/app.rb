# frozen_string_literal: true

module ALI
  module TUI
    # The interactive visualizer: the event loop.
    class App
      # Returns the exit status.
      def self.run(path, input: $stdin, output: $stdout, error: $stderr)
        new(input, output, error).run(path)
      end

      def initialize(input, output, error)
        @in = input
        @out = output
        @err = error
      end

      def run(path)
        unless @in.tty? && @out.tty?
          @err.puts "ali: the visualizer needs a terminal. Use --plain for the text mode."
          return 1
        end

        status = Terminal.open(input: @in, output: @out) do |terminal|
          @terminal = terminal
          path ||= pick_program
          path ? load_and_show(path) : 0
        end
        # The terminal is normal again here, so the errors stay on the screen.
        report_errors(path) if status == 1
        status
      end

      private

      def load_and_show(path)
        loaded = Assembler.assemble_file(path)
        loaded.ok? ? visualize(path, loaded.program) : show_errors(path, loaded.errors)
      end

      # ---- The program list ----------------------------------------------

      def program_files
        local = Dir.glob(File.join(Dir.pwd, "programs", "*.sal"))
        local = Dir.glob(File.expand_path("../../../programs/*.sal", __dir__)) if local.empty?
        local.sort
      end

      def pick_program
        picker = Picker.new(program_files)
        loop do
          @terminal.draw(picker.draw(*@terminal.size))
          @terminal.read_keys(nil).each do |key|
            case key
            when "q", "Escape" then return nil
            when "Up", "k" then picker.move(-1)
            when "Down", "j" then picker.move(1)
            when "Enter" then return picker.chosen if picker.chosen
            end
          end
        end
      end

      # ---- The error screen ----------------------------------------------

      def show_errors(path, errors)
        @errors = errors
        screen = ErrorScreen.new(File.basename(path), errors, File.file?(path) ? File.binread(path) : "")
        loop do
          @terminal.draw(screen.draw(*@terminal.size))
          break if @terminal.read_keys(nil).intersect?(%w[q Escape Enter])
        end
        1
      end

      def report_errors(path)
        @errors&.each { @err.puts "#{path}: #{it}" }
      end

      # ---- The visualizer ------------------------------------------------

      def visualize(path, program)
        controller = Controller.new(program)
        screen = Screen.new(controller, File.basename(path))
        next_tick = nil
        loop do
          @terminal.draw(screen.draw(*@terminal.size))
          next_tick = controller.running? ? next_tick || (now + controller.tick_interval) : nil
          keys = @terminal.read_keys(next_tick && [next_tick - now, 0].max)
          if keys.empty? && next_tick && now >= next_tick
            controller.tick
            next_tick = nil
          end
          keys.each { |key| return 0 if handle(controller, screen, key) == :quit }
        end
      end

      def handle(controller, screen, key)
        case key
        when Terminal::Click then click(controller, screen, key)
        when :resize then nil
        else Keys.handle(controller, key)
        end
      end

      # A click on a program line sets or removes its breakpoint.
      def click(controller, screen, click)
        address = screen.address_at(click.x, click.y)
        return unless address

        controller.select(address)
        controller.toggle_breakpoint(address)
      end

      def now = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    end
  end
end
