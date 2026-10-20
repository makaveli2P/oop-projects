# frozen_string_literal: true

require "prism"

module SetCalc
  # Compiles the text of the "l" command into a callable lambda, with four
  # layers of protection:
  #
  # 1. Prism parses the text. The text must be exactly one lambda literal with
  #    exactly one named parameter: "->(x) { ... }", "-> x { ... }" or
  #    "lambda { |x| ... }".
  # 2. Each node of the body must be on an allowlist: literals, the parameter,
  #    if/unless/and/or, arrays, strings, and calls on a receiver to a method on
  #    an allowlist (operators, a small set of Numeric/String/Array methods, and
  #    Math functions). The body cannot call a method without a receiver
  #    (system, eval, exit, puts), assign a variable, use a block, a loop, a
  #    constant other than Math, a backtick, a global or an instance variable.
  # 3. The calculator evaluates a canonical copy of the lambda in the binding of
  #    an empty BasicObject, so self has no Kernel methods.
  # 4. Lambda#call_all runs the calls in a child process with a time limit, so
  #    an expensive expression (x ** 10**9) cannot freeze the program.
  #
  # Each error is a SafeLambda::Error with a message for the user.
  class SafeLambda
    class Error < StandardError; end

    # The outcome of one call: the value as text, or the error.
    Outcome = Data.define(:input, :text, :error) do
      def ok?
        error.nil?
      end
    end

    OPERATORS = %i[+ - * / % ** == != < > <= >= <=> & | ^ << >> ! -@ +@ ~].freeze
    METHODS = %i[
      abs magnitude succ pred even? odd? zero? positive? negative? nonzero? integer? finite? infinite?
      round floor ceil truncate to_i to_f to_r to_s inspect fdiv div modulo divmod remainder pow
      gcd lcm digits bit_length clamp between? []
      length size reverse upcase downcase capitalize strip center ljust rjust
      first last min max sum include? join sort count empty? uniq
    ].freeze
    MATH_METHODS = %i[sqrt cbrt sin cos tan asin acos atan atan2 exp log log2 log10 hypot].freeze
    MATH_CONSTANTS = %i[PI E].freeze

    ALLOWED_NODES = [
      Prism::StatementsNode, Prism::ParenthesesNode, Prism::IntegerNode, Prism::FloatNode,
      Prism::RationalNode, Prism::ImaginaryNode, Prism::TrueNode, Prism::FalseNode, Prism::NilNode,
      Prism::StringNode, Prism::SymbolNode, Prism::InterpolatedStringNode, Prism::EmbeddedStatementsNode,
      Prism::ArrayNode, Prism::ArgumentsNode, Prism::IfNode, Prism::UnlessNode, Prism::ElseNode,
      Prism::AndNode, Prism::OrNode, Prism::LocalVariableReadNode, Prism::CallNode,
      Prism::ConstantReadNode, Prism::ConstantPathNode
    ].freeze

    TIME_LIMIT = 2.0 # seconds for all the calls of one command
    TEXT_LIMIT = 200 # characters of one result

    # An object without Kernel methods. The lambda is evaluated in its binding.
    class Sandbox < BasicObject
      Math = ::Math

      def __binding
        ::Kernel.binding
      end
    end

    attr_reader :source, :parameter

    def self.compile(text)
      new(text)
    end

    def initialize(text)
      @text = text.to_s
      @parameter, body = parse_lambda(@text)
      check_node(body) if body
      @source = "->(#{@parameter}) { #{body&.slice} }"
      # The source is a canonical lambda whose body passed the allowlist above.
      @callable = ::Kernel.eval(@source, Sandbox.new.__binding, "(lambda)", 1) # rubocop:disable Security/Eval
    end

    def to_s
      @text.strip
    end

    # Calls the lambda in this process. Returns an Outcome.
    def call(value)
      outcome_for(value) { @callable.call(value) }
    end

    # Calls the lambda for each value and returns an array of Outcome. The
    # calls run in a child process when the platform supports fork.
    def call_all(values, time_limit: TIME_LIMIT)
      return values.map { |value| call(value) } unless Process.respond_to?(:fork)

      call_in_child(values, time_limit)
    end

    private

    def parse_lambda(text)
      result = Prism.parse(text)
      raise Error, "syntax error: #{result.errors.first.message}" unless result.success?

      statements = result.value.statements.body
      raise Error, "type exactly one lambda, for example: ->(x) { x * 2 }" unless statements.size == 1

      lambda_parts(statements.first)
    end

    def lambda_parts(node)
      case node
      when Prism::LambdaNode
        [parameter_name(node.parameters), node.body]
      when Prism::CallNode
        unless node.name == :lambda && node.receiver.nil? && node.arguments.nil? && node.block.is_a?(Prism::BlockNode)
          raise Error, "the text is not a lambda. Type a lambda, for example: ->(x) { x * 2 }"
        end

        [parameter_name(node.block.parameters), node.block.body]
      else
        raise Error, "the text is not a lambda. Type a lambda, for example: ->(x) { x * 2 }"
      end
    end

    def parameter_name(parameters)
      list = parameters.is_a?(Prism::BlockParametersNode) ? parameters.parameters : nil
      if list.nil? || !parameters.locals.empty? || !single_required?(list)
        raise Error, "the lambda must have exactly one named parameter, for example: ->(x) { x + 1 }"
      end

      list.requireds.first.name
    end

    def single_required?(list)
      list.requireds.size == 1 && list.requireds.first.is_a?(Prism::RequiredParameterNode) &&
        list.optionals.empty? && list.rest.nil? && list.posts.empty? &&
        list.keywords.empty? && list.keyword_rest.nil? && list.block.nil?
    end

    def check_node(node)
      reject(node) unless ALLOWED_NODES.any? { |type| node.is_a?(type) }

      case node
      when Prism::LocalVariableReadNode then check_variable(node)
      when Prism::CallNode then check_call(node)
      when Prism::ConstantReadNode then check_constant(node)
      when Prism::ConstantPathNode then check_constant_path(node)
      end
      node.compact_child_nodes.each { |child| check_node(child) }
    end

    def check_variable(node)
      reject(node, "only the parameter #{@parameter} is allowed") unless node.name == @parameter && node.depth.zero?
    end

    def check_call(node)
      reject(node, "a method call needs a receiver, for example x.abs") if node.receiver.nil?
      reject(node, "a block is not allowed") if node.block
      if math_receiver?(node.receiver)
        reject(node, "Math.#{node.name} is not allowed") unless MATH_METHODS.include?(node.name)
      elsif !OPERATORS.include?(node.name) && !METHODS.include?(node.name)
        reject(node, "the method #{node.name} is not allowed")
      end
    end

    def check_constant(node)
      reject(node, "the only constant allowed is Math") unless node.name == :Math
    end

    def check_constant_path(node)
      return if math_receiver?(node.parent) && MATH_CONSTANTS.include?(node.name)

      reject(node, "the only constants allowed are Math::PI and Math::E")
    end

    def math_receiver?(node)
      node.is_a?(Prism::ConstantReadNode) && node.name == :Math
    end

    def reject(node, reason = nil)
      snippet = node.slice.lines.first.to_s.strip
      snippet = "#{snippet[0, 40]}..." if snippet.length > 40
      raise Error, ["not allowed in a lambda: #{snippet}", reason].compact.join(" - ")
    end

    def outcome_for(value)
      result = yield
      Outcome.new(input: value, text: limit(result.inspect), error: nil)
    rescue StandardError, ScriptError, SystemStackError, NoMemoryError => e
      Outcome.new(input: value, text: nil, error: limit("#{e.class}: #{e.message}"))
    end

    def limit(text)
      text = text.to_s
      text.length > TEXT_LIMIT ? "#{text[0, TEXT_LIMIT]}..." : text
    end

    def call_in_child(values, time_limit)
      reader, writer = IO.pipe
      pid = Process.fork do
        reader.close
        rows = values.map { |value| call(value) }.map { |o| [o.text, o.error] }
        writer.write(Marshal.dump(rows))
      ensure
        exit!(0) # Skip at_exit hooks of the parent, for example the test runner.
      end
      writer.close
      rows = read_child(reader, pid, time_limit)
      values.zip(rows).map { |value, (text, error)| Outcome.new(input: value, text: text, error: error) }
    ensure
      reader&.close unless reader&.closed?
      writer&.close unless writer&.closed?
    end

    def read_child(reader, pid, time_limit)
      deadline = Process.clock_gettime(Process::CLOCK_MONOTONIC) + time_limit
      data = +""
      loop do
        remaining = deadline - Process.clock_gettime(Process::CLOCK_MONOTONIC)
        break kill_child(pid, "the lambda took more than #{time_limit} seconds") if remaining <= 0

        next unless reader.wait_readable(remaining)

        chunk = reader.read_nonblock(65_536, exception: false)
        break if chunk.nil?

        data << chunk unless chunk == :wait_readable
      end
      Process.wait(pid)
      raise Error, "the lambda stopped the child process" if data.empty?

      Marshal.load(data) # rubocop:disable Security/MarshalLoad -- data from our own child, plain strings
    end

    def kill_child(pid, message)
      Process.kill(:KILL, pid)
      Process.wait(pid)
      raise Error, message
    end
  end
end
