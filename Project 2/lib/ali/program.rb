# frozen_string_literal: true

module ALI
  # One instruction of a program: the instruction object, its source text
  # (without the comment) and its 1-based source line number.
  CodeLine = Data.define(:instruction, :text, :line)

  # A symbol that DEC declares, and the data address that it gets.
  SymbolBinding = Data.define(:name, :address)

  # The immutable result of the assembler.
  #
  # The constructor checks the invariants that Machine relies on. Code that
  # makes a Program without the assembler thus cannot put the machine in a
  # state that it cannot show.
  class Program
    attr_reader :code, :symbols

    def initialize(code, symbols)
      check_code(code)
      check_symbols(symbols)
      @code = code.dup.freeze
      @symbols = symbols.dup.freeze
      @by_name = symbols.to_h { [it.name, it] }.freeze
      @by_address = symbols.to_h { [it.address, it] }.freeze
      freeze
    end

    def size = code.size
    def at(address) = code.fetch(address)
    def address_of(name) = @by_name[name]&.address
    def symbol_at(address) = @by_address[address]&.name

    private

    def check_code(code)
      raise ArgumentError, "a program has 1 to #{MemoryMap::CODE_SIZE} instructions" unless
        code.size.between?(1, MemoryMap::CODE_SIZE)
      raise ArgumentError, "each code line must hold an instruction" unless
        code.all? { it.is_a?(CodeLine) && it.instruction.is_a?(Instruction) }
    end

    def check_symbols(symbols)
      raise ArgumentError, "a program declares at most #{MemoryMap::DATA_SIZE} symbols" if
        symbols.size > MemoryMap::DATA_SIZE
      raise ArgumentError, "symbol names must be unique" unless symbols.map(&:name).uniq.size == symbols.size

      symbols.each_with_index do |symbol, index|
        next if symbol.address == MemoryMap::DATA_BASE + index

        raise ArgumentError, "symbol #{symbol.name} must be at address #{MemoryMap::DATA_BASE + index}"
      end
    end
  end
end
