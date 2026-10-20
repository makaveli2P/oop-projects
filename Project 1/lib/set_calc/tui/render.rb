# frozen_string_literal: true

require_relative "keys"
require_relative "player"
require_relative "screen"

module SetCalc
  module TUI
    # The --render developer mode: it drives a Player with commands and keys,
    # and returns one frame of the screen as text. It needs no terminal.
    module Render
      module_function

      # +commands+: each command executes, and its animation skips to the end,
      # except the last one. +step+: show this step (1-based) of the last
      # command; nil shows the last step. +keys+: a key script, applied after.
      def frame(commands: [], step: nil, keys: "", width: 120, height: 36, color: false)
        player = Player.new
        commands.each_with_index do |command, i|
          player.submit(command)
          player.press(:end) if i < commands.size - 1 || step.nil?
        end
        player.press(:home) if step
        (step.to_i - 1).times { player.press(:right) } if step
        # The app draws after each key, and the draw limits the scroll. Do
        # the same here, so the frame is the frame that the app shows.
        Keys.script(keys).each do |key|
          Screen.render(player, width, height)
          player.press(key)
        end
        Screen.render(player, width, height).to_s(color: color)
      end
    end
  end
end
