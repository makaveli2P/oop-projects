# frozen_string_literal: true

module ALI
  # Finds a program file from the name that the user types.
  module SourceFile
    EXTENSIONS = ["", ".txt", ".sal"].freeze

    module_function

    # Returns the name as typed if that file exists. Otherwise, tries the name
    # with ".txt" and ".sal". Returns nil if no file exists.
    def resolve(name)
      EXTENSIONS.map { name + it }.find { File.file?(it) }
    end
  end

  # The classic text mode of the original course project: a command loop with
  # the commands s (step), a (run all) and q (quit).
  class PlainMode
    PROMPT = "Enter the SAL file name: "
    COMMANDS = "Command (s = step, a = run all, q = quit): "

    def initialize(input: $stdin, output: $stdout, error: $stderr, step_limit: Machine::DEFAULT_STEP_LIMIT)
      @in = input
      @out = output
      @err = error
      @step_limit = step_limit
    end

    # Returns the exit status: 0, or 1 if no program loads.
    def run(file = nil)
      path = file ? SourceFile.resolve(file) || file : ask_for_file
      return 1 unless path

      loaded = Assembler.assemble_file(path)
      return report_errors(path, loaded.errors) unless loaded.ok?

      @machine = Machine.new(loaded.program)
      @out.puts "Loaded #{program.size} instructions and #{program.symbols.size} symbols from #{path}."
      print_code
      command_loop
      0
    end

    private

    def program = @machine.program

    # Reads one line without the outer spaces. Returns nil at the end of the input.
    def read_line = @in.gets&.strip

    def ask_for_file
      loop do
        @out.print PROMPT
        @out.flush
        name = read_line
        if name.nil? || name.empty?
          @err.puts "ali: no file name"
          return nil
        end
        path = SourceFile.resolve(name)
        return path if path

        @out.puts "There is no file '#{name}' (also tried #{name}.txt and #{name}.sal). Try again."
      end
    end

    def report_errors(path, errors)
      errors.each { @err.puts "#{path}: #{it}" }
      1
    end

    def command_loop
      loop do
        @out.print "\n#{COMMANDS}"
        @out.flush
        case read_line&.downcase
        when nil, "q" then break @out.puts
        when "s" then do_step
        when "a" then do_run_all
        when "" then next
        else @out.puts "Unknown command. Use s, a or q."
        end
      end
    end

    def do_step
      step = @machine.step
      return @out.puts("The machine is already halted. Nothing to execute.") unless step.executed?

      line = program.at(step.address)
      @out.puts "\n[step #{@machine.steps}] address #{step.address}: #{line.text}  -- #{line.instruction.describe}"
      step.micro_ops.each { @out.puts "    #{it}" }
      print_state
    end

    # Runs to a halt. After each block of steps, it asks to continue.
    def do_run_all
      return @out.puts("The machine is already halted. Nothing to execute.") if @machine.halted?

      total = 0
      loop do
        result = @machine.run(limit: @step_limit)
        total += result.steps
        break if result.stop == :halted
        break @out.puts unless continue?(total)
      end
      @out.puts "\nExecuted #{total} steps."
      print_state
    end

    def continue?(total)
      @out.print "The program ran #{total} steps and did not halt. Continue? [y/n] "
      @out.flush
      read_line&.downcase&.start_with?("y") || false
    end

    def print_state
      print_registers
      print_code
      print_data
    end

    def print_registers
      m = @machine
      @out.puts "Registers: A = #{m.a}  B = #{m.b}  PC = #{m.pc}  Z = #{MicroOp.bit(m.z)}  V = #{MicroOp.bit(m.v)}"
      @out.puts "The machine is halted: #{MicroOp.halt_text(m.halt_reason)}." if m.halted?
    end

    def print_code
      @out.puts "Code memory:"
      program.code.each_with_index do |line, address|
        marker = !@machine.halted? && address == @machine.pc ? "  -> " : "     "
        @out.puts "#{marker}#{address.to_s.rjust(3)}  #{line.text.ljust(24)} (line #{line.line})"
      end
    end

    def print_data
      @out.puts "Data memory:"
      @out.puts "     (no symbols)" if program.symbols.empty?
      program.symbols.each do |symbol|
        @out.puts "     #{symbol.address.to_s.rjust(3)}  #{symbol.name.ljust(12)} = #{@machine.data(symbol.address)}"
      end
    end
  end
end
