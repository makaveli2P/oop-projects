#!/usr/bin/env ruby
# frozen_string_literal: true

# A set calculator: three sets X, Y and Z, each a binary search tree.
# See README.md. Run: ruby main.rb (visualizer) or ruby main.rb --plain.

require_relative "lib/set_calc"
require_relative "lib/set_calc/cli"

exit SetCalc::CLI.run(ARGV)
