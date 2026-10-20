# frozen_string_literal: true

require_relative "../ali"

module ALI
  # The terminal visualizer. The controller, the routes, the keys and the
  # screens have no terminal I/O; only Terminal and App touch the terminal.
  module TUI
  end
end

require_relative "tui/controller"
require_relative "tui/route"
require_relative "tui/keys"
require_relative "tui/canvas"
require_relative "tui/diagram"
require_relative "tui/screen"
require_relative "tui/other_screens"
require_relative "tui/render"
require_relative "tui/terminal"
require_relative "tui/app"
