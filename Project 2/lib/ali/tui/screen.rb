# frozen_string_literal: true

module ALI
  module TUI
    MIN_WIDTH = 100
    MIN_HEIGHT = 30

    # Draws the main screen of the visualizer on a Canvas.
    #
    #   row 0          the header: the file name, the base, the speed and the state
    #   rows 1 to H-5  the panes: Program, Datapath, Memory
    #   row H-4        the registers and the position (step, micro-operation)
    #   row H-3        the narration of the active micro-operation
    #   row H-2        the message of the last command
    #   row H-1        the keys
    class Screen
      KEYS = [["space", "next"], ["s", "step"], ["b", "back"], ["r", "run"], ["g", "to end"], ["+/-", "speed"],
              ["R", "reset"], ["x", "hex"], ["⏎", "breakpoint"], ["q", "quit"]].freeze

      MAX_DIAGRAM_WIDTH = 66
      REGISTER_WIDTHS = { A: 11, B: 11, PC: 3, Z: 1, V: 1 }.freeze
      # The rows of the instruction card under the diagram.
      CARD_HEIGHT = 4
      START_DETAIL = "Each press shows one micro-operation: one movement of data on the bus."
      # The micro-operation fields that hold names. A long narration makes these shorter.
      NAME_FIELDS = %i[symbol name text operand].freeze

      # The screen rows of the program lines after the last draw: address -> row.
      attr_reader :source_rows
      # The screen columns of the Program pane after the last draw (a Range).
      attr_reader :source_columns

      def initialize(controller, file_name)
        @c = controller
        @file_name = file_name
        @source_rows = {}
        @source_columns = 0...0
        @last_selection = controller.selection
        @focus = 0
        @marker_seen = nil
      end

      # The program address at a screen cell after the last draw, or nil if
      # the cell is not on a program line of the Program pane.
      def address_at(x, y) = (@source_rows.key(y) if @source_columns.cover?(x))

      def draw(width, height)
        @source_rows = {}
        @source_columns = 0...0
        return SmallScreen.new.draw(width, height) if width < MIN_WIDTH || height < MIN_HEIGHT

        canvas = Canvas.new(width, height)
        program_width, memory_width = pane_widths(width)
        pane_height = height - 5
        draw_header(canvas, width)
        draw_program(canvas, 0, 1, program_width, pane_height)
        draw_datapath(canvas, program_width, 1, width - program_width - memory_width, pane_height)
        draw_memory(canvas, width - memory_width, 1, memory_width, pane_height)
        draw_footer(canvas, width, height)
        canvas
      end

      private

      def state = @c.view

      def pane_widths(width) = width >= 120 ? [28, 30] : [25, 26]

      # ---- Header ----------------------------------------------------------

      def draw_header(canvas, width)
        canvas.text(1, 0, "ALI", bold: true).text(5, 0, "· #{@file_name}", :muted)
        status, ink = status_text
        canvas.text_right(width - 1, 0, status, ink, bold: true)
        canvas.text_right(width - 5 - status.length, 0, "#{base_name}  ·  speed #{@c.speed_text} micro-ops/s", :muted)
      end

      def base_name = @c.base == :hex ? "hexadecimal" : "decimal"

      def status_text
        if (reason = @c.halt_reason)
          ink = { hlt_executed: :ok, ran_past_end: :target }.fetch(reason, :alert)
          return ["■ Halted: #{MicroOp.halt_text(reason)}", ink]
        end
        return ["● Running", :target] if @c.running?

        [@c.instruction? ? "Paused" : "Ready", :plain]
      end

      # ---- Program pane ----------------------------------------------------

      def draw_program(canvas, x, y, width, height)
        canvas.frame(x, y, width, height, title: "Program")
        rows = height - 3
        marker_kind, marker_address = marker
        last = @c.program.size # The "end" row.
        first = scroll_start(focus_address(marker_address), last + 1, rows)
        @source_rows = {}
        @source_columns = x...(x + width)
        (first..[last, first + rows - 1].min).each_with_index do |address, index|
          row = y + 2 + index
          @source_rows[address] = row if address < last
          @row_marker = marker_address == address ? marker_kind : nil
          draw_source_line(canvas, x + 1, row, width - 2, address)
        end
      end

      def marker
        return [:executing, @c.instruction_address] if @c.instruction? && !@c.instruction_complete?

        case @c.halt_reason
        when :ran_past_end then [:halted, @c.program.size]
        when nil then [:next, state.pc]
        else [:halted, state.pc]
        end
      end

      # The pane follows the selection after the user moves it, and else the marker.
      def focus_address(marker_address)
        @focus = @c.selection if @c.selection != @last_selection
        @focus = marker_address if @c.selection == @last_selection && @marker_seen != marker_address
        @last_selection = @c.selection
        @marker_seen = marker_address
        @focus
      end

      # The first row to show, so that the focus address is visible with some context.
      def scroll_start(focus, count, rows)
        return 0 if count <= rows

        (focus - (rows / 3)).clamp(0, count - rows)
      end

      def draw_source_line(canvas, x, row, width, address)
        marker_kind = @row_marker
        selected = address == @c.selection && address < @c.program.size
        canvas.text(x, row, " " * width, selected:)
        draw_marker(canvas, x + 1, row, marker_kind, selected)
        canvas.text(x + 2, row, "●", :alert, selected:) if @c.breakpoint?(address)
        canvas.text(x + 3, row, address.to_s.rjust(3), :muted, selected:)
        text = address < @c.program.size ? @c.program.at(address).text : "end"
        ink = address < @c.program.size ? :plain : :muted
        bold = marker_kind == :executing
        canvas.text(x + 8, row, fit(text, width - 8), marker_kind == :executing ? :target : ink, bold:, selected:)
      end

      def draw_marker(canvas, x, row, kind, selected)
        case kind
        when :executing then canvas.text(x, row, "▶", :target, bold: true, selected:)
        when :next then canvas.text(x, row, "▶", bold: true, selected:)
        when :halted
          ink = { hlt_executed: :ok, ran_past_end: :target }.fetch(@c.halt_reason, :alert)
          canvas.text(x, row, "■", ink, bold: true, selected:)
        end
      end

      def fit(text, width) = text.length > width ? "#{text[0, width - 1]}…" : text

      # ---- Datapath pane ---------------------------------------------------

      def draw_datapath(canvas, x, y, width, height)
        canvas.frame(x, y, width, height, title: "Datapath")
        inner = [width - 4, MAX_DIAGRAM_WIDTH].min
        left = x + ((width - inner) / 2)
        block = Diagram::HEIGHT + 2 + CARD_HEIGHT
        top = y + 1 + [(height - 2 - block) / 2, 1].max
        Diagram.new(@c).draw(canvas, left, top, inner)
        draw_card(canvas, left + 2, top + Diagram::HEIGHT + 2, inner - 4)
      end

      # The instruction card: the instruction, its description, and one dot
      # for each micro-operation. Each line stays inside +width+ columns.
      def draw_card(canvas, x, y, width)
        address = @c.instruction_address
        return draw_hint(canvas, x, y, width) if address.nil?

        line = @c.program.at(address)
        dots = @c.micro_ops.each_index.map { it < @c.cursor ? "●" : "○" }.join(" ")
        canvas.text(x, y, fit(line.text, width - dots.length - 1), bold: true)
        canvas.text_right(x + width, y, dots, :target)
        canvas.restyle(x + width - dots.length, y, dots.length, ink: :muted) if @c.cursor.zero?
        lines = wrap(line.instruction.describe, width, 2)
        lines.each_with_index { |text, index| canvas.text(x, y + 1 + index, text) }
        canvas.text(x, y + 1 + lines.size, fit("address #{address}, source line #{line.line}", width), :muted)
      end

      def draw_hint(canvas, x, y, width)
        title, detail = if @c.halted?
                          ["The machine is halted.", "Press R to reset, or b to go back."]
                        else
                          ["Press space to start.", START_DETAIL]
                        end
        canvas.text(x, y, fit(title, width), bold: true)
        wrap(detail, width, CARD_HEIGHT - 1).each_with_index do |text, index|
          canvas.text(x, y + 1 + index, text, :muted)
        end
      end

      # Breaks +text+ into at most +count+ lines of +width+ columns, at spaces.
      # A word that is longer than a line is broken into parts.
      def wrap(text, width, count)
        lines = [+""]
        text.split.each { |word| add_word(lines, word, width) }
        kept = lines.first(count)
        kept[-1] = fit("#{kept[-1]} #{lines[count..].join(" ")}", width) if lines.size > count
        kept
      end

      # Adds +word+ to the last line, or to a new line if it does not fit. A
      # word that is longer than a line fills the last line and continues.
      def add_word(lines, word, width)
        until word.empty?
          gap = lines.last.empty? ? "" : " "
          room = width - lines.last.length - gap.length
          if word.length <= room
            lines.last << gap << word
            return
          end
          if word.length > width && room.positive?
            lines.last << gap << word[0, room]
            word = word[room..]
          end
          lines << +""
        end
      end

      # ---- Memory pane -----------------------------------------------------

      def draw_memory(canvas, x, y, width, height)
        canvas.frame(x, y, width, height, title: "Memory")
        rows = MemoryPane.new(@c, width - 2).rows(height - 3)
        rows.each_with_index do |segments, index|
          column = x + 1
          segments.each do |segment|
            canvas.text(column, y + 2 + index, segment.text, segment.ink, bold: segment.bold)
            column += segment.text.length
          end
        end
      end

      # ---- Footer ----------------------------------------------------------

      def draw_footer(canvas, width, height)
        draw_registers(canvas, width, height - 4)
        draw_narration(canvas, width, height - 3)
        canvas.text(2, height - 2, @c.message, message_ink)
        x = 2
        KEYS.each do |key, label|
          canvas.text(x, height - 1, key, bold: true).text(x + key.length + 1, height - 1, label, :muted)
          x += key.length + label.length + 3
        end
      end

      def message_ink
        return :alert if @c.message.start_with?("Stopped at the breakpoint")
        return :ok if @c.halt_reason == :hlt_executed && @c.message.start_with?("The machine is halted")

        :plain
      end

      def draw_registers(canvas, width, row)
        x = 2
        previous = @c.previous
        [[:A, state.a, previous.a], [:B, state.b, previous.b], [:PC, state.pc, previous.pc],
         [:Z, state.z, previous.z], [:V, state.v, previous.v]].each do |name, value, old|
          text = register_text(name, value)
          changed = value != old
          canvas.text(x, row, name.to_s, :muted).text(x + name.length + 1, row, text, changed ? :target : :plain,
                                                      bold: changed)
          x += name.length + 1 + [text.length, REGISTER_WIDTHS.fetch(name)].max + 5
        end
        position = position_text(width - 2 - x)
        canvas.text_right(width - 2, row, position, :muted)
      end

      # Where the user is in the run: "step N  ·  micro-op k of m". A short
      # form is used when the long form does not fit in +room+ columns.
      def position_text(room)
        step = @c.instruction? ? "step #{@c.step_number}" : "step 0"
        return step unless @c.active_micro_op

        long = "#{step}  ·  micro-op #{@c.cursor} of #{@c.micro_ops.size}"
        long.length <= room ? long : "#{step} · #{@c.cursor}/#{@c.micro_ops.size}"
      end

      def register_text(name, value)
        case name
        when :Z, :V then MicroOp.bit(value)
        when :PC then Word.format_address(value, @c.base)
        else Word.format(value, @c.base)
        end
      end

      # The narration has the full row. The values are in the base of the
      # panes. If the sentence is too long, the names become shorter and the
      # values stay complete.
      def draw_narration(canvas, width, row)
        op = @c.active_micro_op
        return unless op

        canvas.text(2, row, "▸", :target).text(4, row, narration_text(op, width - 6))
      end

      def narration_text(op, room)
        text = op.to_s(@c.base)
        excess = text.length - room
        return text unless excess.positive?

        field = NAME_FIELDS.find { op.respond_to?(it) && op.public_send(it).to_s.length > excess + 1 }
        return fit(text, room) unless field

        name = op.public_send(field)
        fit(op.with(field => "#{name[0, name.length - excess - 1]}…").to_s(@c.base), room)
      end
    end

    # The rows of the Memory pane: the data words with their symbols, then a
    # summary of the code words. Each row is a list of segments.
    class MemoryPane
      Segment = Data.define(:text, :ink, :bold) do
        def initialize(text:, ink: :plain, bold: false) = super
      end

      def initialize(controller, width)
        @c = controller
        @width = width
        @access = access_marks
      end

      # The rows for a pane with +visible+ rows. If the data words do not fit,
      # the pane shows a window of them around the word that the active
      # micro-operation uses.
      def rows(visible)
        tail = [[], [seg(" CODE", :muted)], *code_rows]
        data = data_rows
        budget = visible - 1 - tail.size
        data = window(data, budget) if data.size > budget
        [[seg(" DATA", :muted)], *data.map(&:last), *tail]
      end

      private

      def seg(text, ink = :plain, bold: false) = Segment.new(text:, ink:, bold:)
      def program = @c.program
      def state = @c.view

      # Each data row as [address, segments]. The address is nil for the "free" row.
      def data_rows
        symbols = program.symbols
        rows = symbols.flat_map { |symbol| symbol_rows(symbol).map { [symbol.address, it] } }
        free = MemoryMap::DATA_BASE + symbols.size
        rows << [nil, [seg("  #{range(free, MemoryMap::SIZE - 1)}  free", :muted)]] if free < MemoryMap::SIZE
        rows
      end

      # Keeps +budget+ rows. The first and the last kept row tell how many rows are hidden.
      def window(rows, budget)
        focus = rows.index { |address, _| @access.key?(address) } || 0
        inner = [budget - 2, 1].max
        start = (focus - (inner / 2)).clamp(0, [rows.size - inner, 0].max)
        kept = rows[start, inner]
        hidden_after = rows.size - start - kept.size
        kept.unshift([nil, [seg("  ↑ #{start} more", :muted)]]) if start.positive?
        kept.push([nil, [seg("  ↓ #{hidden_after} more", :muted)]]) if hidden_after.positive?
        kept
      end

      # One row with the mark, the address, the name and the value. If they do
      # not fit, the value goes on a second row, so that the pane shows the
      # full name.
      def symbol_rows(symbol)
        mark, ink = @access.fetch(symbol.address, [" ", :plain])
        head = [seg(mark, ink, bold: true), seg(symbol.address.to_s.rjust(4), :muted), seg(" ")]
        value = value_segment(symbol.address, ink)
        room = @width - 6 - value.text.length - 2
        if symbol.name.length <= room
          return [[*head, seg(symbol.name.ljust(room), ink, bold: ink != :plain), seg(" "), value]]
        end

        long = @width - 7
        name = symbol.name.length > long ? "#{symbol.name[0, long - 1]}…" : symbol.name
        [[*head, seg(name, ink, bold: ink != :plain)], [seg(" " * (@width - value.text.length - 1)), value]]
      end

      # The value of a data word. A value that changed in this step is amber.
      def value_segment(address, ink)
        changed = state.memory[address] != @c.previous.memory[address]
        seg(Word.format(state.memory[address], @c.base), changed ? :target : ink, bold: changed || ink != :plain)
      end

      def code_rows
        size = program.size
        rows = [[seg("  #{range(0, size - 1).rjust(8)}  program")]]
        if size < MemoryMap::CODE_SIZE
          rows << [seg("  #{range(size, MemoryMap::CODE_SIZE - 1).rjust(8)}  empty", :muted)]
        end
        rows
      end

      def range(low, high) = low == high ? low.to_s : "#{low}–#{high}"

      # The data address that the active micro-operation reads (▸), writes (◂)
      # or tells about (•).
      def access_marks
        case @c.active_micro_op
        in MicroOp::MemRead(address:) then { address => ["▸", :source] }
        in MicroOp::MemWrite(address:) then { address => ["◂", :target] }
        in MicroOp::SymbolInfo(address:) then { address => ["•", :source] }
        else {}
        end
      end
    end

    # The screen for a terminal that is too small.
    class SmallScreen
      def draw(width, height)
        canvas = Canvas.new(width, height)
        lines = ["ALI needs a terminal of at least #{MIN_WIDTH} × #{MIN_HEIGHT}.",
                 "This terminal is #{width} × #{height}. Please enlarge it.", "", "q quit"]
        top = [(height - lines.size) / 2, 0].max
        lines.each_with_index do |line, index|
          canvas.text([(width - line.length) / 2, 0].max, top + index, line, index.zero? ? :plain : :muted,
                      bold: index.zero?)
        end
        canvas
      end
    end
  end
end
