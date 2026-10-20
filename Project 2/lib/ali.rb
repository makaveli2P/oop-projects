# frozen_string_literal: true

# ALI: an interpreter and a terminal visualizer for SAL (Simple Assembly
# Language).
#
# The core (this file and the files that it requires) does no terminal I/O.
# ALI::PlainMode and ALI::TUI are the two front ends.
module ALI
  VERSION = "2.0.0"
end

require_relative "ali/word"
require_relative "ali/memory_map"
require_relative "ali/micro_op"
require_relative "ali/instructions"
require_relative "ali/program"
require_relative "ali/assembler"
require_relative "ali/datapath"
require_relative "ali/machine"
