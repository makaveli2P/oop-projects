# frozen_string_literal: true

module ALI
  module TUI
    # The hardware units on the datapath diagram.
    UNITS = %i[PC Memory IR Decoder A B ALU Z V].freeze

    # Where a micro-operation moves data: the source units, the target units
    # and the value that moves.
    Route = Data.define(:sources, :targets, :label) do
      def self.none = new(sources: [], targets: [], label: "")

      def source?(unit) = sources.include?(unit)
      def target?(unit) = targets.include?(unit)
      def empty? = sources.empty? && targets.empty?
      def units = sources | targets
    end

    # Finds the route of the micro-operation at +index+ in +ops+. The other
    # micro-operations of the same instruction tell where a value comes from
    # or goes to. For example, the RegWrite of LDA gets its value from Memory,
    # the RegWrite of ADD from the ALU, and the RegWrite of LDI from the
    # Decoder.
    module Routing
      include MicroOp

      module_function

      def route_of(ops, index, base = :decimal) # rubocop:disable Metrics/CyclomaticComplexity, Metrics/PerceivedComplexity
        word = ->(value) { Word.format(value, base) }
        case ops.fetch(index)
        in Fetch(text:) then route(%i[PC Memory], [:IR], text)
        in Decode(opcode:) then route([:IR], [:Decoder], opcode)
        in RegRead(reg:, value:) then route([reg], [read_target(ops, index, reg)], word.call(value))
        in RegWrite(reg:, new_value:) then route([write_source(ops, index, reg)], [reg], word.call(new_value))
        in FlagRead(flag:, value:) then route([flag], [:Decoder], "#{flag} = #{MicroOp.bit(value)}")
        in FlagWrite(flag:, new_value:) then route([:ALU], [flag], MicroOp.bit(new_value))
        in MemRead(value:) then route([:Memory], [read_register(ops, index)], word.call(value))
        in MemWrite(new_value:) then route([:A], [:Memory], word.call(new_value))
        in AluAdd(result:) then route(%i[A B], [:ALU], word.call(result))
        in PcWrite => op then pc_route(op, base)
        in SymbolInfo(address:) then route([:Decoder], [:Memory], address.to_s)
        in Halt then Route.none
        end
      end

      def route(sources, targets, label) = Route.new(sources:, targets:, label:)

      def pc_route(op, base)
        label = base == :hex ? format("0x%02X", op.new_value) : op.new_value.to_s
        route(op.jump? ? [:Decoder] : [], [:PC], label)
      end

      # Where a register write gets its value.
      def write_source(ops, index, target)
        earlier = ops.first(index)
        return :ALU if earlier.any?(AluAdd) # ADD
        return :Memory if earlier.any?(MemRead) # LDA, LDB
        return (target == :A ? :B : :A) if earlier.any? { |op| op.is_a?(RegRead) && op.reg != target } # XCH

        :Decoder # LDI: the value is in the instruction.
      end

      # Where a register read sends its value: the first later micro-operation
      # that is not a register read tells it.
      def read_target(ops, index, source)
        ops.drop(index + 1).each do |op|
          case op
          when AluAdd then return :ALU
          when MemWrite then return :Memory
          when RegWrite then return source == :A ? :B : :A
          when RegRead then next
          else break
          end
        end
        :ALU
      end

      # The register that receives a word from memory: the next micro-operation tells it.
      def read_register(ops, index)
        case ops[index + 1]
        in RegWrite(reg:) then reg
        else :A
        end
      end
    end
  end
end
