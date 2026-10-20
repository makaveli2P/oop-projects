# frozen_string_literal: true

module ALI
  # The layout of the 256-word memory.
  module MemoryMap
    SIZE = 256
    CODE_SIZE = 128
    DATA_BASE = 128
    DATA_SIZE = SIZE - DATA_BASE

    module_function

    def data?(address) = address.between?(DATA_BASE, SIZE - 1)
    def code?(address) = address.between?(0, CODE_SIZE - 1)
  end
end
