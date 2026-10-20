# frozen_string_literal: true

module ALI
  # The next value of the PC that an instruction asks for.
  #   :increment, :branch_not_taken -> PC + 1
  #   :jump, :branch_taken          -> target
  #   :halt                         -> the PC keeps its value
  NextPc = Data.define(:cause, :target) do
    def initialize(cause:, target: nil) = super
  end

  # The abstract base class of the 11 SAL instructions.
  #
  # Each subclass names its opcode and its operand kind with the class macro
  # +opcode+. The assembler uses these to parse a line. Each subclass also has:
  #
  # * +execute(datapath)+: the behavior. It changes the machine only through
  #   the Datapath, so each change becomes a micro-operation. It returns a
  #   NextPc.
  # * +describe+: one sentence for the visualizer and the plain mode.
  class Instruction
    NEXT = NextPc.new(cause: :increment)

    class << self
      attr_reader :mnemonic, :operand_kind

      private

      # Declares the opcode of a subclass. +operand+ is :none, :symbol,
      # :integer or :address.
      def opcode(mnemonic, operand: :none)
        @mnemonic = mnemonic
        @operand_kind = operand
      end
    end

    def mnemonic = self.class.mnemonic

    # The operand as text. Empty when the instruction has no operand.
    def operand = ""

    def to_s = operand.empty? ? mnemonic : "#{mnemonic} #{operand}"

    def execute(_datapath) = raise(NotImplementedError, "#{self.class} must implement execute")

    def describe = raise(NotImplementedError, "#{self.class} must implement describe")

    def ==(other) = other.class == self.class && other.to_s == to_s
    alias eql? ==

    def hash = [self.class, to_s].hash
  end

  # The base class of DEC, LDA, LDB and STR: instructions that name a symbol.
  # The assembler resolves the symbol to its data address.
  class SymbolInstruction < Instruction
    attr_reader :symbol, :address

    def initialize(symbol, address)
      super()
      @symbol = symbol
      @address = address
      freeze
    end

    def operand = symbol

    private

    def at_address = "#{symbol} (address #{address})"
  end

  # The base class of JMP, JZS and JVS.
  class JumpInstruction < Instruction
    attr_reader :target

    def initialize(target)
      super()
      @target = target
      freeze
    end

    def operand = target.to_s

    private

    def branch_if(datapath, flag)
      datapath.read_flag(flag) ? NextPc.new(cause: :branch_taken, target:) : NextPc.new(cause: :branch_not_taken)
    end
  end

  # The base class of XCH, ADD and HLT: instructions without an operand.
  class PlainInstruction < Instruction
    def initialize
      super
      freeze
    end
  end

  # DEC symbol: declares a symbol. The assembler gives the address at load
  # time, so at run time DEC changes no data. It only tells the address.
  class Dec < SymbolInstruction
    opcode "DEC", operand: :symbol

    def describe = "Declare symbol #{at_address}"

    def execute(datapath)
      datapath.report_symbol(symbol, address)
      NEXT
    end
  end

  # LDA symbol: A <- memory[symbol].
  class Lda < SymbolInstruction
    opcode "LDA", operand: :symbol

    def describe = "Load A from #{at_address}"

    def execute(datapath)
      datapath.write_reg(:A, datapath.read_mem(address))
      NEXT
    end
  end

  # LDB symbol: B <- memory[symbol].
  class Ldb < SymbolInstruction
    opcode "LDB", operand: :symbol

    def describe = "Load B from #{at_address}"

    def execute(datapath)
      datapath.write_reg(:B, datapath.read_mem(address))
      NEXT
    end
  end

  # STR symbol: memory[symbol] <- A.
  class Str < SymbolInstruction
    opcode "STR", operand: :symbol

    def describe = "Store A into #{at_address}"

    def execute(datapath)
      datapath.write_mem(address, datapath.read_reg(:A))
      NEXT
    end
  end

  # LDI value: A <- value.
  class Ldi < Instruction
    opcode "LDI", operand: :integer

    attr_reader :value

    def initialize(value)
      super()
      raise ArgumentError, "#{value} is not a 32-bit integer" unless value.is_a?(Integer) && Word.fits?(value)

      @value = value
      freeze
    end

    def operand = value.to_s
    def describe = "Load the value #{value} into A"

    def execute(datapath)
      datapath.write_reg(:A, value)
      NEXT
    end
  end

  # XCH: swaps A and B.
  class Xch < PlainInstruction
    opcode "XCH"

    def describe = "Exchange A and B"

    def execute(datapath)
      a = datapath.read_reg(:A)
      b = datapath.read_reg(:B)
      datapath.write_reg(:A, b)
      datapath.write_reg(:B, a)
      NEXT
    end
  end

  # ADD: A <- A + B. Sets Z and V.
  class Add < PlainInstruction
    opcode "ADD"

    def describe = "Add B to A; set Z and V"

    def execute(datapath)
      sum = datapath.alu_add(datapath.read_reg(:A), datapath.read_reg(:B))
      datapath.write_reg(:A, sum.result)
      datapath.write_flag(:Z, sum.zero)
      datapath.write_flag(:V, sum.overflow)
      NEXT
    end
  end

  # HLT: halts the machine. The PC keeps the address of HLT.
  class Hlt < PlainInstruction
    opcode "HLT"

    def describe = "Halt the machine"
    def execute(_datapath) = NextPc.new(cause: :halt)
  end

  # JMP number: PC <- number.
  class Jmp < JumpInstruction
    opcode "JMP", operand: :address

    def describe = "Jump to address #{target}"
    def execute(_datapath) = NextPc.new(cause: :jump, target:)
  end

  # JZS number: PC <- number if Z is 1.
  class Jzs < JumpInstruction
    opcode "JZS", operand: :address

    def describe = "Jump to address #{target} if Z is set"
    def execute(datapath) = branch_if(datapath, :Z)
  end

  # JVS number: PC <- number if V is 1.
  class Jvs < JumpInstruction
    opcode "JVS", operand: :address

    def describe = "Jump to address #{target} if V is set"
    def execute(datapath) = branch_if(datapath, :V)
  end

  # The instruction set: the opcode in upper case -> the instruction class.
  INSTRUCTION_SET = [Dec, Lda, Ldb, Ldi, Str, Xch, Jmp, Jzs, Jvs, Add, Hlt].to_h { [it.mnemonic, it] }.freeze
end
