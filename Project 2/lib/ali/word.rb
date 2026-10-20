# frozen_string_literal: true

module ALI
  # Signed 32-bit words. Ruby integers have no size limit, so the machine
  # uses these helpers to keep each register and each data word in 32 bits.
  module Word
    BITS = 32
    MIN = -(2**(BITS - 1))
    MAX = (2**(BITS - 1)) - 1
    MODULUS = 2**BITS

    module_function

    # Returns true when +value+ fits in a signed 32-bit word.
    def fits?(value) = value.between?(MIN, MAX)

    # Returns the two's-complement 32-bit value of +value+.
    def wrap(value) = ((value - MIN) % MODULUS) + MIN

    # Formats a word in decimal or as 8 hexadecimal digits.
    def format(value, base = :decimal)
      base == :hex ? Kernel.format("0x%08X", value % MODULUS) : value.to_s
    end

    # Formats a number that can be outside the 32-bit range, for example the
    # exact sum of the ALU. In hexadecimal, a value outside the range keeps its
    # sign and all of its digits.
    def format_wide(value, base = :decimal)
      return format(value, base) if fits?(value) || base != :hex

      "#{"-" if value.negative?}0x#{value.abs.to_s(16).upcase}"
    end

    # Formats an address (0 to 255): in hexadecimal, two digits.
    def format_address(value, base = :decimal)
      base == :hex ? Kernel.format("0x%02X", value) : value.to_s
    end
  end

  # The result of the ALU addition. +wide_sum+ is the exact sum.
  AddResult = Data.define(:result, :wide_sum, :zero, :overflow)

  # The arithmetic logic unit. It has one operation: 32-bit addition.
  module ALU
    module_function

    # Adds two words. The result wraps to 32 bits. +overflow+ is true when the
    # exact sum is outside the 32-bit range. +zero+ is true when the 32-bit
    # result is 0.
    def add(left, right)
      wide = left + right
      result = Word.wrap(wide)
      AddResult.new(result:, wide_sum: wide, zero: result.zero?, overflow: result != wide)
    end
  end
end
