# frozen_string_literal: true

module ALI
  # The only way for an instruction to change the machine.
  #
  # Each method reads or changes the working state and adds one
  # micro-operation to the trace. This is the hook point for the visualizer:
  # the order of the calls in Instruction#execute is the order of the
  # micro-operations on the screen.
  class Datapath
    # An access to an address that is not a data address. Machine#step
    # catches it and halts the machine with the reason :runtime_fault.
    class Fault < StandardError
      attr_reader :address

      def initialize(address)
        @address = address
        super("address #{address} is not a data address")
      end
    end

    REGISTERS = %i[A B].freeze
    FLAGS = %i[Z V].freeze

    # The working state after the calls so far.
    attr_reader :state

    def initialize(state, program, trace)
      @state = state
      @program = program
      @trace = trace
    end

    def read_reg(reg)
      value = state.public_send(register_key(reg))
      @trace << MicroOp::RegRead.new(reg:, value:)
      value
    end

    def write_reg(reg, value)
      key = register_key(reg)
      @trace << MicroOp::RegWrite.new(reg:, old_value: state.public_send(key), new_value: value)
      @state = state.with(key => value)
    end

    def read_flag(flag)
      value = state.public_send(flag_key(flag))
      @trace << MicroOp::FlagRead.new(flag:, value:)
      value
    end

    def write_flag(flag, value)
      key = flag_key(flag)
      @trace << MicroOp::FlagWrite.new(flag:, old_value: state.public_send(key), new_value: value)
      @state = state.with(key => value)
    end

    def read_mem(address)
      check_data(address)
      value = state.memory[address]
      @trace << MicroOp::MemRead.new(address:, symbol: @program.symbol_at(address), value:)
      value
    end

    def write_mem(address, value)
      check_data(address)
      @trace << MicroOp::MemWrite.new(address:, symbol: @program.symbol_at(address),
                                      old_value: state.memory[address], new_value: value)
      @state = state.write_memory(address, value)
    end

    def alu_add(left, right)
      sum = ALU.add(left, right)
      @trace << MicroOp::AluAdd.new(a: left, b: right, wide_sum: sum.wide_sum, result: sum.result)
      sum
    end

    def report_symbol(name, address)
      @trace << MicroOp::SymbolInfo.new(name:, address:)
    end

    private

    def register_key(reg)
      raise ArgumentError, "unknown register #{reg.inspect}" unless REGISTERS.include?(reg)

      reg.downcase
    end

    def flag_key(flag)
      raise ArgumentError, "unknown flag #{flag.inspect}" unless FLAGS.include?(flag)

      flag.downcase
    end

    def check_data(address)
      raise Fault, address unless address.is_a?(Integer) && MemoryMap.data?(address)
    end
  end
end
