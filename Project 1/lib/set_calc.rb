# frozen_string_literal: true

# A set calculator: three sets of numbers X, Y and Z, each stored in a binary
# search tree, with a classic command loop and a terminal visualizer.
module SetCalc
end

require_relative "set_calc/node"
require_relative "set_calc/bst"
require_relative "set_calc/command"
require_relative "set_calc/safe_lambda"
require_relative "set_calc/trace"
require_relative "set_calc/calculator"
require_relative "set_calc/classic"
