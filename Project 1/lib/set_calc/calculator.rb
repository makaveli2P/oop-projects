# frozen_string_literal: true

require_relative "bst"
require_relative "command"
require_relative "safe_lambda"
require_relative "trace"

module SetCalc
  # The result of one command.
  #
  # +summary+ is one sentence about the change. +output+ is the text that the
  # command prints (the results of "l", or a note such as "7 is already in X").
  Result = Data.define(:summary, :output, :error, :quit) do
    def self.ok(summary, output = [])
      new(summary: summary, output: output, error: false, quit: false)
    end

    def self.failure(message)
      new(summary: message, output: [], error: true, quit: false)
    end

    def self.quit
      new(summary: "Goodbye.", output: [], error: false, quit: true)
    end
  end

  # The set calculator: three sets X, Y and Z, each a BST, and the commands of
  # the assignment. The calculator tells a tracer about each step, so the
  # visualizer can animate the command. The classic mode uses a NullTracer.
  class Calculator
    SET_NAMES = CommandParser::SET_NAMES
    MAX_ELEMENTS = 1000
    RESULT_TITLE = "X ∩ Y"

    def initialize
      @sets = SET_NAMES.to_h { |name| [name, BST.new] }
      @scratch = nil # The tree that "i" builds, before it becomes X.
      @trace = NullTracer.new
    end

    def [](name)
      @sets.fetch(name)
    end

    # [name, title, tree] for each tree that a frame shows.
    def panels
      list = SET_NAMES.map { |name| [name, name.to_s, @sets[name]] }
      list << [:R, RESULT_TITLE, @scratch] if @scratch
      list
    end

    # Parses and executes one line. Never raises for bad input.
    def run(line, tracer: NullTracer.new)
      execute(CommandParser.parse(line), tracer: tracer)
    rescue CommandError, SafeLambda::Error => e
      Result.failure(e.message)
    end

    def execute(command, tracer: NullTracer.new)
      @trace = tracer
      dispatch(command)
    ensure
      @scratch = nil
      @trace = NullTracer.new
    end

    private

    def dispatch(command)
      case command.name
      when *SET_NAMES then assign(command.name, command.argument)
      when :a then add(command.argument)
      when :r then rotate
      when :s then switch
      when :u then union
      when :i then intersection
      when :c then deep_copy
      when :l then apply_lambda(command.argument)
      when :q then Result.quit
      end
    end

    # X values: X becomes a new tree with the values, in the order given.
    def assign(name, values)
      check_unique_count(name, values)
      tree = BST.new
      @sets[name] = tree
      count = values.size == 1 ? "1 value" : "#{values.size} values"
      @trace.step("#{name} #{values.join(',')}: make #{name} a new empty tree, then insert #{count} in order.",
                  tags: { name => "new" })
      duplicates = values.reject { |value| traced_insert(name, tree, value) }
      notes = duplicates.map { |value| "#{value} is a duplicate. #{name} has it only once." }
      @trace.finish("#{name} = #{tree}", marks: { name => marks_for(tree.to_a, :new) })
      Result.ok("#{name} = #{tree}", notes)
    end

    # a i: insert i into X.
    def add(value)
      tree = @sets[:X]
      @trace.step("a #{value}: insert #{value} into X. Start at the root.")
      if tree.search(value)
        traced_insert(:X, tree, value)
        @trace.finish("#{value} is already in X. X does not change.")
        return Result.ok("#{value} is already in X.", ["#{value} is already in X."])
      end

      check_limit(:X, tree.size + 1)
      traced_insert(:X, tree, value)
      @trace.finish("X = #{tree}", marks: { X: { value => :new } })
      Result.ok("Inserted #{value} into X.")
    end

    # r: Y takes X, Z takes Y, X takes the old Z.
    def rotate
      @trace.step("r: rotate. Y takes the content of X, Z takes Y, and X takes the old Z.",
                  tags: { X: "▸ to Y", Y: "▸ to Z", Z: "▸ to X" })
      @sets[:X], @sets[:Y], @sets[:Z] = @sets[:Z], @sets[:X], @sets[:Y]
      @trace.finish("Rotated. X has the old Z, Y has the old X, Z has the old Y.",
                    tags: { X: "◂ from Z", Y: "◂ from X", Z: "◂ from Y" })
      Result.ok("Rotated the sets: X ← Z, Y ← X, Z ← Y.")
    end

    # s: X and Y change places.
    def switch
      @trace.step("s: switch. X and Y change places.", tags: { X: "▸ to Y", Y: "▸ to X" })
      @sets[:X], @sets[:Y] = @sets[:Y], @sets[:X]
      @trace.finish("Switched. X has the old Y, Y has the old X.", tags: { X: "◂ from Y", Y: "◂ from X" })
      Result.ok("Switched X and Y.")
    end

    # u: X = X ∪ Y. Insert each element of Y into X, in pre-order.
    def union
      x = @sets[:X]
      y = @sets[:Y]
      check_limit(:X, x.size + y.count { |value| !x.include?(value) })
      @trace.step("u: union. Visit each node of Y in pre-order and insert its value into X.")
      visited = []
      added = 0
      y.each_preorder do |value|
        y_marks = { Y: marks_for(visited, :done).merge(value => :focus) }
        @trace.step("Visit #{value} in Y. Insert #{value} into X.", marks: y_marks) if @trace.recording?
        added += 1 if traced_insert(:X, x, value, y_marks)
        visited << value
      end
      @trace.finish("X = X ∪ Y = #{x}", marks: { Y: marks_for(visited, :done) }, tags: { X: "◂ X ∪ Y" })
      Result.ok("X = X ∪ Y. Added #{added} of #{y.size} elements of Y.")
    end

    # i: X = X ∩ Y. Visit X in pre-order, search Y for each value, and insert
    # each value that Y has into a new tree. Then the new tree becomes X.
    def intersection
      x = @sets[:X]
      y = @sets[:Y]
      result = BST.new
      @scratch = result
      @trace.step("i: for each node of X in pre-order, search Y. Insert each value that Y has into a new tree.",
                  tags: { R: "new" })
      visited = []
      x.each_preorder do |value|
        x_marks = { X: marks_for(visited, :done).merge(value => :focus) }
        @trace.step("Visit #{value} in X. Search for #{value} in Y.", marks: x_marks) if @trace.recording?
        traced_insert(:R, result, value, x_marks) if traced_search(:Y, y, value, x_marks)
        visited << value
      end
      @trace.step("The new tree is X ∩ Y = #{result}. It becomes the new X.", tags: { X: "▸ old X", R: "▸ to X" })
      @scratch = nil
      @sets[:X] = result
      @trace.finish("X = X ∩ Y = #{result}", tags: { X: "◂ X ∩ Y" })
      Result.ok("X = X ∩ Y. Kept #{result.size} of #{x.size} elements.")
    end

    # c: Y becomes a recursive deep copy of X.
    def deep_copy
      x = @sets[:X]
      @sets[:Y] = BST.new
      @trace.step("c: deep copy X into Y. Copy a node, then its left subtree, then its right subtree.",
                  tags: { Y: "new" })
      copied = []
      copy = x.deep_copy do |value, partial, parent, side|
        @sets[:Y] = partial
        if @trace.recording?
          where = parent ? "as the #{side} child of #{parent}" : "as the root"
          @trace.step("Copy #{value} into Y #{where}.",
                      marks: { X: marks_for(copied, :done).merge(value => :focus), Y: { value => :new } })
        end
        copied << value
      end
      @sets[:Y] = copy
      @trace.finish("Y = #{copy}. Y has new nodes in the same shape as X.", tags: { Y: "◂ copy of X" })
      Result.ok("Y is a deep copy of X.")
    end

    # l aString: call the lambda for each element of X, in order. X does not
    # change.
    def apply_lambda(text)
      function = SafeLambda.compile(text)
      x = @sets[:X]
      @trace.step("l: call #{function} for each element of X, in ascending order.")
      outcomes = function.call_all(x.to_a)
      done = []
      outcomes.each do |outcome|
        if @trace.recording?
          shown = outcome.ok? ? outcome.text : outcome.error
          @trace.step("f(#{outcome.input}) = #{shown}",
                      marks: { X: marks_for(done, :done).merge(outcome.input => :focus) })
        end
        done << outcome.input
      end
      list = "[#{outcomes.map { |o| o.ok? ? o.text : "<#{o.error[/\A.+?(?=: )/] || o.error}>" }.join(', ')}]"
      errors = outcomes.reject(&:ok?).map { |o| "f(#{o.input}) raised #{o.error}" }
      count = outcomes.size == 1 ? "1 element" : "#{outcomes.size} elements"
      @trace.finish("Called the lambda for #{count} of X. X does not change.")
      Result.ok("Results: #{list}", [list, *errors])
    end

    # Inserts the value and tells the tracer about each comparison. +context+
    # holds the marks of the other panels. Returns true if the tree changed.
    def traced_insert(name, tree, value, context = {})
      path = []
      tree.insert(value) do |event, *args|
        next unless @trace.recording?

        if event == :compare
          node_value, side = args
          @trace.step(compare_text(name, value, node_value, side),
                      marks: context.merge(name => marks_for(path, :path).merge(node_value => :focus)))
          path << node_value
        else
          _, parent, side = args
          text = if parent.nil?
                   "#{label(name)} is empty. Put #{value} as the root."
                 else
                   "Put #{value} as the #{side} child of #{parent}."
                 end
          @trace.step(text, marks: context.merge(name => marks_for(path, :path).merge(value => :new)))
        end
      end
    end

    # Searches the tree for "i" and tells the tracer about each comparison.
    def traced_search(name, tree, value, context = {})
      path = []
      found = tree.search(value) do |_event, node_value, side|
        next unless @trace.recording?

        text = if side == :equal
                 "#{value} = #{node_value}: found #{value} in #{label(name)}. Insert #{value} into #{RESULT_TITLE}."
               else
                 "#{order_text(value, node_value, side)}, go #{side}."
               end
        mark = side == :equal ? :new : :focus
        @trace.step(text, marks: context.merge(name => marks_for(path, :path).merge(node_value => mark)))
        path << node_value
      end
      if !found && @trace.recording?
        @trace.step("#{value} is not in #{label(name)}: the search reached an empty place. Skip #{value}.",
                    marks: context.merge(name => marks_for(path, :path)))
      end
      found
    end

    def compare_text(name, value, node_value, side)
      return "#{value} = #{node_value}: #{value} is already in #{label(name)}. Skip it." if side == :equal

      "#{order_text(value, node_value, side)}, go #{side}."
    end

    def order_text(value, node_value, side)
      "#{value} #{side == :left ? '<' : '>'} #{node_value}"
    end

    def label(name)
      name == :R ? RESULT_TITLE : name.to_s
    end

    def marks_for(values, role)
      values.to_h { |value| [value, role] }
    end

    # Counts the distinct values with a scratch tree (the tree compares 1 and
    # 1.0 as equal, as the set does). The count stops at the first value past
    # the limit, so a long sorted list cannot freeze the program.
    def check_unique_count(name, values)
      unique = BST.new
      values.each do |value|
        unique.insert(value)
        check_limit(name, unique.size)
      end
    end

    def check_limit(name, size)
      return if size <= MAX_ELEMENTS

      raise CommandError, "#{name} can hold at most #{MAX_ELEMENTS} elements"
    end
  end
end
