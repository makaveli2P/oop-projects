# frozen_string_literal: true

module ALI
  # A load error. +line+ and +column+ are 1-based; 0 means "not known".
  ParseError = Data.define(:line, :column, :message) do
    def to_s
      return message unless line.positive?

      where = column.positive? ? "line #{line}, column #{column}" : "line #{line}"
      "#{where}: #{message}"
    end
  end

  # The result of the load step: a Program, or a list of errors.
  AssembleResult = Data.define(:program, :errors) do
    def ok? = !program.nil?
  end

  # The SAL load step.
  #
  # Pass 1 splits the source into lines and tokens, counts the instructions
  # and gives each DEC symbol a data address. Pass 2 makes one Instruction
  # object for each instruction line and resolves the operands. The
  # assembler never stops at the first error: it reports all of them, in the
  # order of the lines.
  class Assembler
    Token = Data.define(:text, :column)
    SourceLine = Data.define(:number, :text, :tokens)

    SYMBOL = /\A[A-Za-z]+\z/
    INTEGER = /\A[+-]?\d+\z/
    ADDRESS = /\A\d+\z/
    OPERAND_COUNT = { none: 0, symbol: 1, integer: 1, address: 1 }.freeze

    def self.assemble(source) = new(source).run

    def self.assemble_file(path)
      assemble(File.binread(path))
    rescue SystemCallError, IOError => e
      AssembleResult.new(program: nil, errors: [ParseError.new(0, 0, "cannot open file '#{path}' (#{e.message})")])
    end

    def initialize(source)
      @source = source.dup.force_encoding(Encoding::UTF_8).scrub
      @errors = []
      @symbols = []
    end

    def run
      @lines = split_lines
      allocate_symbols
      code = @lines.filter_map do |line|
        instruction = build(line)
        CodeLine.new(instruction:, text: line.text, line: line.number) if instruction
      end
      @errors << ParseError.new(0, 0, "the program contains no instructions") if @lines.empty?
      return failure unless @errors.empty?

      AssembleResult.new(program: Program.new(code, @symbols), errors: [])
    end

    private

    def failure
      errors = @errors.each_with_index.sort_by { |e, index| [e.line, e.column, index] }.map(&:first)
      AssembleResult.new(program: nil, errors: errors.freeze)
    end

    def error(line, column, message) = @errors << ParseError.new(line, column, message)

    # Keeps only the lines that hold an instruction. A comment starts at ';'
    # or '#'. Blank lines and comment lines take no address.
    def split_lines
      source = @source.delete_prefix("﻿")
      lines = []
      source.each_line(chomp: true).with_index(1) do |raw, number|
        line = tokenize(number, raw)
        next if line.tokens.empty?

        if lines.size == MemoryMap::CODE_SIZE
          error(number, line.tokens.first.column, "the program has more than #{MemoryMap::CODE_SIZE} instructions")
        end
        lines << line
      end
      lines
    end

    def tokenize(number, raw)
      code = raw[/\A[^;#]*/]
      tokens = []
      code.scan(/\S+/) { tokens << Token.new(Regexp.last_match[0], Regexp.last_match.begin(0) + 1) }
      SourceLine.new(number:, text: code.strip, tokens:)
    end

    def opcode_of(token) = INSTRUCTION_SET[token.text.upcase]

    def find_symbol(name) = @symbols.find { it.name == name }

    # Pass 1 for DEC: data addresses 128, 129, ... in the order of the DEC lines.
    def allocate_symbols
      @lines.each do |line|
        next unless opcode_of(line.tokens.first) == Dec && line.tokens.size == 2

        name = line.tokens[1]
        next unless SYMBOL.match?(name.text)

        if find_symbol(name.text)
          error(line.number, name.column, "duplicate declaration of symbol '#{name.text}'")
        elsif @symbols.size == MemoryMap::DATA_SIZE
          error(line.number, name.column, "the program declares more than #{MemoryMap::DATA_SIZE} symbols")
        else
          @symbols << SymbolBinding.new(name: name.text, address: MemoryMap::DATA_BASE + @symbols.size)
        end
      end
    end

    # Pass 2: makes the instruction object for one line, or reports errors.
    def build(line)
      op_token, *operands = line.tokens
      klass = opcode_of(op_token)
      unless klass
        error(line.number, op_token.column, "unknown opcode '#{op_token.text}'")
        return nil
      end
      return nil unless operand_count_ok?(line, klass, operands)

      case klass.operand_kind
      when :none then klass.new
      when :integer then build_ldi(line, operands.first)
      when :address then build_jump(line, klass, operands.first)
      when :symbol then build_symbol(line, klass, operands.first)
      end
    end

    def operand_count_ok?(line, klass, operands)
      expected = OPERAND_COUNT.fetch(klass.operand_kind)
      return true if operands.size == expected

      column = operands.size > expected ? operands[expected].column : line.tokens.first.column
      noun = expected == 1 ? "operand" : "operands"
      error(line.number, column, "#{klass.mnemonic} expects #{expected} #{noun}, found #{operands.size}")
      false
    end

    def build_ldi(line, token)
      value = INTEGER.match?(token.text) ? Integer(token.text, 10) : nil
      return Ldi.new(value) if value && Word.fits?(value)

      error(line.number, token.column, "'#{token.text}' is not a 32-bit signed integer")
      nil
    end

    def build_jump(line, klass, token)
      limit = [@lines.size, MemoryMap::CODE_SIZE].min
      target = ADDRESS.match?(token.text) ? Integer(token.text, 10) : nil
      return klass.new(target) if target && target < limit

      error(line.number, token.column, "jump target '#{token.text}' is not an instruction address (0-#{limit - 1})")
      nil
    end

    def build_symbol(line, klass, token)
      # Pass 1 rejected this DEC and told why.
      return nil if klass == Dec && SYMBOL.match?(token.text) && !find_symbol(token.text)

      unless SYMBOL.match?(token.text)
        error(line.number, token.column, "'#{token.text}' is not a valid symbol (use letters only)")
        return nil
      end
      symbol = find_symbol(token.text)
      return klass.new(symbol.name, symbol.address) if symbol

      error(line.number, token.column, "undeclared symbol '#{token.text}'")
      nil
    end
  end
end
