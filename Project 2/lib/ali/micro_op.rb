# frozen_string_literal: true

module ALI
  # The hardware-level trace of one SAL step.
  #
  # Each SAL instruction executes as a short sequence of micro-operations. A
  # micro-operation names one hardware unit (Memory, PC, IR, Decoder, A, B,
  # ALU, Z, V) and one movement of data. Each micro-operation is an immutable
  # Data object with two methods:
  #
  # * +to_s(base)+ gives one English sentence. The values are in +base+
  #   (:decimal, the default, or :hex). The plain mode and the visualizer
  #   print it.
  # * +apply(state)+ gives the machine state after the micro-operation. The
  #   visualizer uses it to show an instruction one micro-operation at a time.
  module MicroOp
    HALT_REASONS = {
      hlt_executed: "HLT executed",
      ran_past_end: "ran past end of program",
      runtime_fault: "runtime fault"
    }.freeze

    PC_CAUSES = {
      increment: "increment",
      jump: "jump",
      branch_taken: "branch taken",
      branch_not_taken: "branch not taken",
      halt: "halt"
    }.freeze

    module_function

    def halt_text(reason) = HALT_REASONS.fetch(reason)

    def memory_name(address, symbol) = symbol ? "Memory[#{address}] (#{symbol})" : "Memory[#{address}]"

    def bit(flag) = flag ? "1" : "0"

    # The default behavior: a micro-operation that only reads changes nothing.
    module ReadOnly
      def apply(state) = state
    end

    # The control unit copies the instruction at +address+ into the IR.
    Fetch = Data.define(:pc, :address, :text) do
      include ReadOnly

      def to_s(_base = :decimal) = %(Fetch: Memory[#{address}] -> IR: "#{text}")
    end

    # The decoder splits the IR into an opcode and an operand.
    Decode = Data.define(:opcode, :operand) do
      include ReadOnly

      def to_s(_base = :decimal)
        operand.empty? ? "Decode: opcode #{opcode}" : "Decode: opcode #{opcode}, operand #{operand}"
      end
    end

    # A register puts its value on the bus.
    RegRead = Data.define(:reg, :value) do
      include ReadOnly

      def to_s(base = :decimal) = "Read #{reg}: #{Word.format(value, base)}"
    end

    # A register latches a new value.
    RegWrite = Data.define(:reg, :old_value, :new_value) do
      def to_s(base = :decimal) = "Write #{reg}: #{Word.format(old_value, base)} -> #{Word.format(new_value, base)}"
      def apply(state) = state.with(reg.downcase => new_value)
    end

    # A branch instruction tests a status bit.
    FlagRead = Data.define(:flag, :value) do
      include ReadOnly

      def to_s(_base = :decimal) = "Test #{flag}: #{MicroOp.bit(value)}"
    end

    # A status bit latches a new value.
    FlagWrite = Data.define(:flag, :old_value, :new_value) do
      def to_s(_base = :decimal) = "Set #{flag}: #{MicroOp.bit(old_value)} -> #{MicroOp.bit(new_value)}"
      def apply(state) = state.with(flag.downcase => new_value)
    end

    # Memory puts the word at +address+ on the bus.
    MemRead = Data.define(:address, :symbol, :value) do
      include ReadOnly

      def to_s(base = :decimal) = "Read #{MicroOp.memory_name(address, symbol)}: #{Word.format(value, base)}"
    end

    # Memory latches a new word at +address+.
    MemWrite = Data.define(:address, :symbol, :old_value, :new_value) do
      def to_s(base = :decimal)
        "Write #{MicroOp.memory_name(address, symbol)}: #{Word.format(old_value, base)} -> " \
          "#{Word.format(new_value, base)}"
      end

      def apply(state) = state.write_memory(address, new_value)
    end

    # The ALU adds two words. +wide_sum+ is the exact sum and +result+ is the
    # 32-bit result.
    AluAdd = Data.define(:a, :b, :wide_sum, :result) do
      include ReadOnly

      def to_s(base = :decimal)
        text = "ALU: #{Word.format(a, base)} + #{Word.format(b, base)} = #{Word.format_wide(wide_sum, base)}"
        wide_sum == result ? text : "#{text}, wraps to #{Word.format(result, base)}"
      end
    end

    # The PC latches a new value.
    PcWrite = Data.define(:old_value, :new_value, :cause) do
      def to_s(base = :decimal)
        "PC: #{Word.format_address(old_value, base)} -> #{Word.format_address(new_value, base)} " \
          "(#{PC_CAUSES.fetch(cause)})"
      end

      def apply(state) = state.with(pc: new_value)
      def jump? = %i[jump branch_taken].include?(cause)
    end

    # DEC tells the data address that the assembler gave to a symbol.
    SymbolInfo = Data.define(:name, :address) do
      include ReadOnly

      def to_s(_base = :decimal) = "Symbol #{name} is at address #{address}"
    end

    # The machine stops. For a runtime fault, +fault_address+ is the address
    # that the instruction tried to use.
    Halt = Data.define(:reason, :fault_address) do
      def initialize(reason:, fault_address: nil) = super

      def to_s(_base = :decimal)
        text = "Halt: #{MicroOp.halt_text(reason)}"
        fault_address ? "#{text} (address #{fault_address} is not a data address)" : text
      end

      def apply(state) = state.with(halt_reason: reason)
    end
  end
end
