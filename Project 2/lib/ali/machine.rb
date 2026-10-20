# frozen_string_literal: true

module ALI
  # An immutable snapshot of the machine: the registers, the flags, the 256
  # memory words, the halt reason and the step counter.
  #
  # A code word (0-127) holds a CodeLine, or nil after the end of the program.
  # A data word (128-255) holds an Integer. Because a State is immutable, a
  # snapshot costs nothing, and the visualizer keeps one for each step.
  State = Data.define(:a, :b, :pc, :z, :v, :memory, :halt_reason, :steps) do
    def self.initial(program)
      memory = Array.new(MemoryMap::SIZE) do |address|
        MemoryMap.data?(address) ? 0 : program.code[address]
      end
      new(a: 0, b: 0, pc: 0, z: false, v: false, memory: memory.freeze, halt_reason: nil, steps: 0)
    end

    def halted? = !halt_reason.nil?

    # Returns a copy of the state with one data word changed.
    def write_memory(address, value)
      memory = self.memory.dup
      memory[address] = value
      with(memory: memory.freeze)
    end
  end

  # The result of one step: the address of the instruction and its
  # micro-operations in order. A step on a halted machine executes nothing.
  StepResult = Data.define(:address, :micro_ops) do
    def executed? = !micro_ops.empty?
  end

  # The result of Machine#run: the number of steps, and :halted or :step_limit.
  RunResult = Data.define(:steps, :stop)

  # The SAL machine. It executes one instruction for each step.
  class Machine
    DEFAULT_STEP_LIMIT = 1000

    attr_reader :program, :state

    def initialize(program)
      @program = program
      reset
    end

    def reset
      @state = State.initial(program)
      self
    end

    # Puts the machine back in an earlier state of the same program.
    def restore(state)
      @state = state
      self
    end

    def a = state.a
    def b = state.b
    def pc = state.pc
    def z = state.z
    def v = state.v
    def steps = state.steps
    def halt_reason = state.halt_reason
    def halted? = state.halted?

    # The value of a data word, or nil for a code word.
    def data(address) = MemoryMap.data?(address) ? state.memory[address] : nil

    # The value of a symbol, or nil when the program does not declare it.
    def value_of(name)
      address = program.address_of(name)
      address && data(address)
    end

    # Executes one instruction: fetch, decode, execute and the PC update.
    def step
      return StepResult.new(address: pc, micro_ops: []) if halted?

      address = pc
      line = program.at(address)
      trace = [MicroOp::Fetch.new(pc:, address: pc, text: line.text),
               MicroOp::Decode.new(opcode: line.instruction.mnemonic, operand: line.instruction.operand)]
      datapath = Datapath.new(state, program, trace)
      next_pc, fault = execute(line.instruction, datapath)
      @state = finish(datapath.state.with(steps: steps + 1), next_pc, fault, trace)
      StepResult.new(address:, micro_ops: trace.freeze)
    end

    # Executes steps until the machine halts or +limit+ steps are done.
    def run(limit: DEFAULT_STEP_LIMIT)
      count = 0
      until halted?
        return RunResult.new(steps: count, stop: :step_limit) if count >= limit

        step
        count += 1
      end
      RunResult.new(steps: count, stop: :halted)
    end

    private

    # A fault stops the instruction before it changes more state.
    def execute(instruction, datapath)
      [instruction.execute(datapath), nil]
    rescue Datapath::Fault => e
      [nil, e.address]
    end

    def finish(state, next_pc, fault, trace)
      old_pc = state.pc
      return halt(state, trace, old_pc, :runtime_fault, fault) if fault
      return halt(state, trace, old_pc, :hlt_executed) if next_pc.cause == :halt

      new_pc = %i[jump branch_taken].include?(next_pc.cause) ? next_pc.target : old_pc + 1
      trace << MicroOp::PcWrite.new(old_value: old_pc, new_value: new_pc, cause: next_pc.cause)
      return state.with(pc: new_pc) if new_pc < program.size

      trace << MicroOp::Halt.new(reason: :ran_past_end)
      state.with(pc: new_pc, halt_reason: :ran_past_end)
    end

    def halt(state, trace, old_pc, reason, fault_address = nil)
      trace << MicroOp::PcWrite.new(old_value: old_pc, new_value: old_pc, cause: :halt)
      trace << MicroOp::Halt.new(reason:, fault_address:)
      state.with(halt_reason: reason)
    end
  end
end
