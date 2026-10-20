# frozen_string_literal: true

module ALI
  module TUI
    # Draws the datapath: the units PC, Memory, IR and Decoder above one
    # internal bus, and A, B, ALU, Z and V below it.
    #
    # The active micro-operation colors its source units cyan and its target
    # units amber. It also colors the part of the bus between them amber, and
    # writes the value that moves next to the bus. An arrow points to the
    # target.
    class Diagram
      HEIGHT = 10
      TOP_ROW = 0     # Rows 0-2: PC, Memory, IR, Decoder.
      BUS_ROW = 4     # Row 3: stubs. Row 4: the bus.
      LABEL_ROW = 5   # Rows 5-6: stubs; row 5 also holds the value.
      LABEL_MAX = 20  # A longer label (a long instruction) ends with "…".
      BOTTOM_ROW = 7  # Rows 7-9: A, B, ALU, Z, V.

      Box = Struct.new(:unit, :x, :width, :top, :stub, keyword_init: true) do
        def center = x + (width / 2)
        def right = x + width
      end

      def initialize(controller)
        @c = controller
        @state = controller.view
        @previous = controller.previous
        op_index = controller.cursor - 1
        @route = op_index.negative? ? Route.none : Routing.route_of(controller.micro_ops, op_index, controller.base)
      end

      def draw(canvas, left, top, width)
        @canvas = canvas
        @left = left
        @top = top
        @boxes = layout(width)
        draw_bus
        @boxes.each { draw_box(it) }
        draw_label(width)
      end

      private

      # ---- Layout ----------------------------------------------------------

      def layout(width)
        gap = width < 70 ? 2 : 3
        wide = ((width - 8 - 9 - (3 * gap)) / 2).clamp(10, 24)
        top = justify([box(:PC, 8, true), box(:Memory, 9, true), box(:IR, wide, true), box(:Decoder, wide, true)],
                      width)
        alu = width >= 54 ? 9 : 5
        reg = ((width - alu - 10 - 12) / 2).clamp(13, 16)
        bottom = justify([box(:A, reg, false), box(:B, reg, false), box(:ALU, alu, false), box(:Z, 5, false),
                          box(:V, 5, false)], width)
        top.each { it.stub = it.center }
        bottom.each { it.stub = free_stub(it, top.map(&:stub)) }
        top + bottom
      end

      def box(unit, width, top) = Box.new(unit:, width:, top:, x: 0)

      # Puts boxes on one row with equal gaps, and centers the row.
      def justify(boxes, width)
        used = boxes.sum(&:width)
        gap = ((width - used) / (boxes.size - 1)).clamp(1, 5)
        x = [(width - used - (gap * (boxes.size - 1))) / 2, 0].max
        boxes.each do |b|
          b.x = x
          x += b.width + gap
        end
      end

      # Moves the stub of a lower box away from the stubs of the upper boxes,
      # so that two stubs never meet the bus at near columns.
      def free_stub(box, upper)
        ((box.x + 2)..(box.right - 3)).min_by do |column|
          near = upper.any? { (it - column).abs <= 2 }
          [near ? 1 : 0, (column - box.center).abs]
        end
      end

      # ---- Drawing ---------------------------------------------------------

      def put(x, y, text, ink = :plain, **) = @canvas.text(@left + x, @top + y, text, ink, **)

      def active?(unit) = @route.source?(unit) || @route.target?(unit)

      def ink_of(unit)
        return :target if @route.target?(unit)
        return :source if @route.source?(unit)

        :muted
      end

      def path_columns = @boxes.select { active?(it.unit) }.map(&:stub)

      def draw_bus
        stubs = @boxes.map(&:stub)
        path = path_columns
        (stubs.min..stubs.max).each do |x|
          on_path = path.size > 1 && x.between?(path.min, path.max)
          put(x, BUS_ROW, junction(x), on_path ? :target : :muted)
        end
        @boxes.each { draw_stub(it) }
      end

      # The bus character at column +x+: a junction where a stub meets the
      # bus, and a corner at each end.
      def junction(x)
        kind = (@boxes.any? { it.top && it.stub == x } ? 1 : 0) + (@boxes.any? { !it.top && it.stub == x } ? 2 : 0)
        return "─" if kind.zero?

        stubs = @boxes.map(&:stub)
        set = if x == stubs.min then "└┌├"
              elsif x == stubs.max then "┘┐┤"
              else "┴┬┼"
              end
        set[kind - 1]
      end

      def draw_stub(box)
        rows = box.top ? [BUS_ROW - 1] : [LABEL_ROW, BOTTOM_ROW - 1]
        ink = active?(box.unit) ? :target : :muted
        rows.each { put(box.stub, it, "│", ink) }
        return unless @route.target?(box.unit)

        put(box.stub, box.top ? BUS_ROW - 1 : BOTTOM_ROW - 1, box.top ? "▲" : "▼", :target, bold: true)
      end

      def draw_box(box)
        y = box.top ? TOP_ROW : BOTTOM_ROW
        ink = ink_of(box.unit)
        title = box.unit.to_s
        edge = "─" * (box.width - 2)
        put(box.x, y, "┌#{edge}┐", ink)
        put(box.x, y + 1, "│#{" " * (box.width - 2)}│", ink)
        put(box.x, y + 2, "└#{edge}┘", ink)
        title = " #{title} " if title.length + 4 <= box.width
        put(box.x + 1, box.top ? y : y + 2, title, ink == :muted ? :plain : ink, bold: true)
        put(box.stub, box.top ? y + 2 : y, box.top ? "┬" : "┴", active?(box.unit) ? :target : ink)
        draw_value(box, y + 1)
      end

      def draw_value(box, y)
        value, align, changed = contents(box.unit)
        inner = box.width - 2
        text = value.length > inner ? "#{value[0, inner - 1]}…" : value
        pad = text.length <= inner - 2 ? 1 : 0
        x = case align
            when :right then box.right - 1 - pad - text.length
            when :center then box.x + ((box.width - text.length) / 2)
            else box.x + 1 + pad
            end
        put(x, y, text, changed ? :target : :plain, bold: changed)
      end

      # The text in a box, its alignment, and true if the value changed in this step.
      def contents(unit)
        case unit
        when :PC then [address(@state.pc), :right, @state.pc != @previous.pc]
        when :A, :B then register(unit)
        when :Z, :V then flag(unit)
        when :ALU then ["+", :center, false]
        else [visible.fetch(unit.downcase, ""), :left, false] # Memory, IR, Decoder
        end
      end

      def register(unit)
        key = unit.downcase
        value = @state.public_send(key)
        [Word.format(value, @c.base), :right, value != @previous.public_send(key)]
      end

      def flag(unit)
        key = unit.downcase
        value = @state.public_send(key)
        [MicroOp.bit(value), :center, value != @previous.public_send(key)]
      end

      def address(value) = @c.base == :hex ? format("0x%02X", value) : value.to_s

      # What the IR, the decoder and the memory port show after the visible micro-operations.
      def visible
        @visible ||= @c.micro_ops.first(@c.cursor).each_with_object({}) do |op, shown|
          case op
          in MicroOp::Fetch(address:, text:)
            shown.merge!(ir: text, memory: "[#{address}]")
          in MicroOp::Decode(opcode:, operand:)
            shown[:decoder] = operand.empty? ? opcode : "#{opcode} #{operand}"
          in MicroOp::MemRead | MicroOp::MemWrite | MicroOp::SymbolInfo
            shown[:memory] = "[#{op.address}]"
          else nil
          end
        end
      end

      # Writes the value that moves on the bus, with an arrow to the target.
      def draw_label(width)
        return if @route.empty? || @route.label.empty?

        text = labelled_value(width)
        path = path_columns
        x = label_position(text, path.min, path.max, width)
        put(x, LABEL_ROW, text, :target, bold: true)
      end

      # The label with an arrow. A long label (the text of a long instruction)
      # becomes shorter, so that the label stays inside the diagram.
      def labelled_value(width)
        label = @route.label
        label = "#{label[0, LABEL_MAX - 1]}…" if label.length > LABEL_MAX
        label = "#{label[0, width - 3]}…" if label.length > width - 2
        sources = @boxes.select { @route.source?(it.unit) }.map(&:stub)
        targets = @boxes.select { @route.target?(it.unit) }.map(&:stub)
        return label if sources.empty?

        mean = ->(columns) { columns.sum.fdiv(columns.size) }
        mean.call(targets) >= mean.call(sources) ? "#{label} ▶" : "◀ #{label}"
      end

      # Finds a place on the label row that is between the ends of the path
      # and covers no stub. If no such place exists, centers the label on the path.
      def label_position(text, low, high, width)
        stubs = @boxes.reject(&:top).map(&:stub)
        fits = (0..(width - text.length)).select do |x|
          stubs.none? { ((x - 1)..(x + text.length)).cover?(it) }
        end
        inside = fits.select { it >= low - 1 && it + text.length <= high + 2 }
        center = ((low + high) / 2) - (text.length / 2)
        best = (inside.empty? ? fits : inside).min_by { (it - center).abs }
        best || center.clamp(0, [width - text.length, 0].max)
      end
    end
  end
end
