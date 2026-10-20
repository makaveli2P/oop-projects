# frozen_string_literal: true

module SetCalc
  # An immutable copy of the shape of a tree. Frames keep snapshots, so the
  # visualizer can show any earlier step of a command.
  Snapshot = Data.define(:value, :left, :right) do
    def self.of(node)
      return nil if node.nil?

      new(value: node.value, left: of(node.left), right: of(node.right))
    end
  end

  # One tree as a frame shows it.
  #
  # +marks+ maps a value of the tree to a role:
  #   :focus   the node that the step looks at now
  #   :path    a node on the path from the root to the focus
  #   :new     a node that the step added
  #   :done    a node that the command already processed
  # +tag+ is a short note next to the title, for example "◂ from X".
  Panel = Data.define(:name, :title, :snapshot, :values, :height, :marks, :tag) do
    def size
      values.size
    end

    def empty?
      values.empty?
    end
  end

  # One step of the animation of a command: all the panels and one sentence.
  Frame = Data.define(:panels, :narration, :command) do
    def panel(name)
      panels.find { |panel| panel.name == name }
    end
  end

  # Records the frames of one command. The calculator calls #step for each
  # step and #finish at the end.
  #
  # A frame keeps a snapshot of each tree. A tree that did not change since the
  # last frame shares the same snapshot, so a long animation is cheap. After
  # +budget+ frames, the tracer records no more steps, only the final frame.
  class Tracer
    DEFAULT_BUDGET = 2000

    attr_reader :frames

    def initialize(calculator, command: nil, budget: DEFAULT_BUDGET)
      @calculator = calculator
      @command = command
      @budget = budget
      @frames = []
      @truncated = false
      @cache = {}
    end

    # False after the budget. Callers check it before they compute marks.
    def recording?
      return true if @frames.size < @budget

      @truncated = true
      false
    end

    def truncated?
      @truncated
    end

    def step(narration, marks: {}, tags: {})
      @frames << build(narration, marks, tags) if recording?
    end

    def finish(narration, marks: {}, tags: {})
      note = @truncated ? " (The animation skips the later steps.)" : ""
      @frames << build("#{narration}#{note}", marks, tags)
    end

    private

    def build(narration, marks, tags)
      panels = @calculator.panels.map do |name, title, tree|
        snapshot, values, height = view_of(tree)
        Panel.new(name: name, title: title, snapshot: snapshot, values: values, height: height,
                  marks: marks.fetch(name, {}).freeze, tag: tags[name])
      end
      Frame.new(panels: panels.freeze, narration: narration, command: @command)
    end

    def view_of(tree)
      key = tree.object_id
      cached = @cache[key]
      return cached[1] if cached && cached[0].equal?(tree) && cached[2] == tree.version

      view = [Snapshot.of(tree.root), tree.to_a.freeze, tree.height].freeze
      @cache[key] = [tree, view, tree.version]
      view
    end
  end

  # A tracer that records nothing. The classic mode uses it.
  class NullTracer
    def recording?
      false
    end

    def step(*, **); end

    def finish(*, **); end

    def frames
      []
    end
  end
end
