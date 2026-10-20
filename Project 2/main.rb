# frozen_string_literal: true

# The entry point of the course project: "ruby main.rb" starts ALI.
# The same program is at bin/ali.
#
#   ruby main.rb [FILE]           the terminal visualizer
#   ruby main.rb --plain [FILE]   the classic s/a/q command loop
require_relative "lib/ali/cli"

exit ALI::CLI.start(ARGV)
