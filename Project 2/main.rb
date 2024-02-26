# Content :   Assembly Language Interpreter (ALI) for a Simple Assembly Language (SAL)

# DESIGN - I have followed two principles : a) Subclass specialized Superclass ,
#                                           b) Inheritance of superclass objects by subclasses.
# Based on these principles, I have 3 Main Classes: a) Hardware - implements memory and related methods, this is passed as reference in the SAL superclass
#                                                   b) SAL - Superclass for all instructions
#                                                   c) ALI - Interpreter that executes user command.
# NOTE: All instructions like DEC, STR, etc are implemented as subclasses of SAL superclass

# Inheritance: Command subclasses DEC, STR,etc inherit instance variables and 'execute' & 'Initialize' method from Superclass SAL
# I refine the 'execute' method in each sub-class and implement specialized instructions

# Flow: ALI class creates Hardware as instances ---> takes user input ---> executes instructions by using Instruction object
# --->  all instruction object uses same hardware instance


class Hardware                              # Hardware class for all the hardware component as per the spec

  attr_accessor :memory, :symbolTable, :register, :programCounter, :zero_result_bit, :overflow_bit, :memory_pointer, :program_data_pointer

  def initialize
    @memory = Array.new(256,0)              # memory array of length 256, initialized with zeros
    @symbolTable = Hash.new                 # symbol hash to store the symbols
    @register = {"A" => 0, "B" => 0}        # hash for register A and register B with initial values 0
    @programCounter = 0                     # program counter variable to hold the address of the next instruction to be executed
    @zero_result_bit = 0                    # zero result bit to be set if ADD instruction produced a zero result
    @overflow_bit = 0                       # overflow bit to be set if ADD instruction produces result more than 2^31
    @memory_pointer = 0                     # memory pointer that points to next empty index in the memory array
    @program_data_pointer = 128             # program data pointer that points to the start of the memory's program data part from 128
  end

  def pc_increment                          # method to increment the program counter after instruction execution
    if @programCounter < @memory_pointer    # limit range for the program counter to only work in memory's range
      @programCounter += 1
    else
      print "end of memory"             # print error message if program counter is at the end of memory array range
    end
  end

  def pd_increment
    @program_data_pointer += 1              # increment program data pointer to point to the next available memory location
  end

end

class SAL                                   # abstract superclass SAL for SAL instructions

  def initialize (hardwareref)
    @hrd = hardwareref                      # initializing an instance variable for hardware class in SAL so that every instruction subclass has access to hardware components
  end

  def execute(argument)                     # deferred method execute to be defined in each instruction subclass
  end

end

class DEC < SAL                                                # subclass for instruction DEC, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                        # refining execute method specific for concrete subclass DEC
    @hrd.symbolTable[argument] = @hrd.program_data_pointer     # creates a new key in the symbol table hash for the symbol associated with DEC and assigns a valid memory location
    @hrd.pc_increment                                          # increments the program counter in the end
    @hrd.pd_increment                                          # increments the program data pointer in the end
  end

end

class LDA < SAL                                                       # subclass for instruction LDA, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                               # refining execute method specific for concrete subclass LDA
    @hrd.register["A"] = @hrd.memory[@hrd.symbolTable[argument]]      # takes the value of the symbol argument from the symbol table hash and loads it in register A
    @hrd.pc_increment                                                 # increments the program counter in the end
  end

end

class LDB < SAL                                                       # subclass for instruction LDB, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                               # refining execute method specific for concrete subclass LDB
    @hrd.register["B"] = @hrd.memory[@hrd.symbolTable[argument]]      # takes the value of the symbol argument from the symbol table hash and loads it in register B
    @hrd.pc_increment                                                 # increments the program counter in the end
  end

end

class LDI < SAL                                        # subclass for instruction LDI, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                # defining execute method specific for concrete subclass LDI
    @hrd.register["A"] = argument.to_i                 # loads the integer value as per the argument into register A
    @hrd.pc_increment                                  # increments the program counter in the end
  end

end

class STR < SAL                                         # subclass for instruction STR, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                               # defining execute method specific for concrete subclass STR
    @hrd.memory[@hrd.symbolTable[argument]] = @hrd.register["A"]      # stores the value in register A to the corresponding symbol in symbol table hash
    @hrd.pc_increment                                                 # increments the program counter in the end
  end

end

class XCH < SAL                                                                         # subclass for instruction XCH, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                                                 # defining execute method specific for concrete subclass XCH
    @hrd.register["A"],@hrd.register["B"] = @hrd.register["B"],@hrd.register["A"]       # exchanges the values of register A and register B through parallel assignment
    @hrd.pc_increment                                                                   # increments the program counter in the end
  end

end

class JMP < SAL                                   # subclass for instruction JMP, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                           # defining execute method specific for concrete subclass JMP
    @hrd.programCounter = argument.to_i           # sets the program counter value as per the argument in the source code and returns it as an integer
    return @hrd.programCounter
  end

end

class JZS < SAL                                   # subclass for instruction JZS, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                           # defining execute method specific for concrete subclass JZS
    if @hrd.zero_result_bit == 1                  # if conditional to check if the zero result bit is set or not
      @hrd.programCounter = argument.to_i
    else
      @hrd.pc_increment                           # if not set, increment the program counter continue execution as per source code sequence
    end
    return  @hrd.programCounter
  end

end

class JVS < SAL                                   # subclass for instruction JVS, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                           # defining execute method specific for concrete subclass JVS
    if @hrd.overflow_bit == 1                     # if conditional to check if the overflow bit is set or not
      @hrd.programCounter = argument.to_i
    else
      @hrd.pc_increment                           # if not set, increment the program counter and continue execution as per source code sequence
    end
    return @hrd.programCounter
  end

end

class ADD < SAL                                                    # subclass for instruction ADD, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)                                            # defining execute method specific for concrete subclass ADD

    sum = @hrd.register["A"].to_i + @hrd.register["B"].to_i        # computing the sum of register A and register B
    @hrd.register["A"] = sum                                       # storing the result in register A

    if sum == 0
      @hrd.zero_result_bit = 1                                     # setting the zero result bit as 1 if the computed sum equals zero
    else
      @hrd.zero_result_bit = 0                                     # clearing the zero result and overflow bit if the add instruction produces a legal result
    end

    if sum > (2**31) or sum < -(2**31)
      @hrd.overflow_bit = 1                                        # setting the overflow bit as 1 if the computed sum is greater than 2147483647 ( Equivalent 2's complement 32-bit representation: 0111 1111 1111 1111 1111 1111 1111 1111)
      @hrd.register["A"] = sum - 2**31                             # storing the overflow value in register A
      else
        @hrd.overflow_bit = 0                                      # clearing the overflow bit if the add instruction produces a legal result
    end
    @hrd.pc_increment                                              # increments the program counter in the end
  end
end

class HLT < SAL                     # subclass for instruction HLT, it inherits SAL (deferred method 'execute' AND the instance variable @hrd in initialize)

  def execute(argument)             # defining execute method specific for concrete subclass HLT
    @hrd.programCounter = @hrd.memory_pointer + 1
  end
end

class ALI                      # interpreter class

  def initialize

    print "Enter the SAL file name: "                            # valid input names are without the .txt extension, in my current directory there are 'test1', 'test2', 'test3'
    file_name = gets.chomp
    @parsed_input = IO.readlines("#{file_name}.txt", chomp: true)    # storing the parsed source code after prompting the user for the file name

    @hrd = Hardware.new                                              # creating a new instance of Hardware class

    @hrd.memory_pointer = @parsed_input.length
    @hrd.memory[0...@hrd.memory_pointer] = @parsed_input             # adding the parsed input line by line to memory indexes using the memory pointer

    @dec = DEC.new @hrd      #initializing instance variable @dec in ALI
    @lda = LDA.new @hrd      #initializing instance variable @lda in ALI
    @ldb = LDB.new @hrd      #initializing instance variable @ldb in ALI
    @ldi = LDI.new @hrd      #initializing instance variable @ldi in ALI
    @str = STR.new @hrd      #initializing instance variable @str in ALI
    @xch = XCH.new @hrd      #initializing instance variable @xch in ALI
    @jmp = JMP.new @hrd      #initializing instance variable @jmp in ALI
    @jzs = JZS.new @hrd      #initializing instance variable @jzs in ALI
    @jvs = JVS.new @hrd      #initializing instance variable @jvs in ALI
    @add = ADD.new @hrd      #initializing instance variable @add in ALI
    @hlt = HLT.new @hrd      #initializing instance variable @hlt in ALI
  end

  def commandLoop                                         # method for the interface/command loop to be experienced by the user

    puts "\nPlease select a command:"
    puts "s : Execute a single line of code"
    puts "a : Execute all the instructions"
    puts "q : Quit the command loop"

    print "\nUser Input: "
    user_input = gets.chomp                               # prompt to get the user input to begin execution

    while user_input!= 'q'
    if user_input == 's'

      if @hrd.programCounter < @hrd.memory_pointer        # command s won't work outside the memory pointer's range, which is ideal
        puts "The code to be executed is"
        puts @hrd.memory_pointer
        puts @hrd.memory[0...@hrd.memory_pointer]
        puts "\n--------------------------------------------------------------------------"
        input_str = @hrd.memory[@hrd.programCounter]
        opcode, arg = input_str.split(' ')                # splitting the code string in the memory into opcode and argument, this way it will work with more than multi length symbols,number and values

        if opcode == "DEC"                                # matching the opcode in the current memory location to the instruction type
          @dec.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
            puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "LDA"                             # matching the opcode in the current memory location to the instruction type
          @lda.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "LDB"                             # matching the opcode in the current memory location to the instruction type
          @ldb.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "LDI"                             # matching the opcode in the current memory location to the instruction type
          @ldi.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "STR"                             # matching the opcode in the current memory location to the instruction type
          @str.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "XCH"                             # matching the opcode in the current memory location to the instruction type
          @xch.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "JMP"                             # matching the opcode in the current memory location to the instruction type
          @jmp.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode== "JZS"                              # matching the opcode in the current memory location to the instruction type
          @jzs.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Program Counter at #{@hrd.programCounter+1}Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode== "JVS"                              # matching the opcode in the current memory location to the instruction type
          @jvs.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "ADD"                             # matching the opcode in the current memory location to the instruction type
          @add.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\nProgram Counter at #{@hrd.programCounter+1}, next intruction executed will be: #{@hrd.memory[@hrd.programCounter]}"
          puts "--------------------------------------------------------------------------"

        elsif opcode == "HLT"                             # matching the opcode in the current memory location to the instruction type
          @hlt.execute(arg)                               # executing the method defined in the concrete subclass which takes the argument as per the source code
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end     # binding the value of the key in symbol table hash with the value at the corresponding to the memory array location
          print "Data Memory: #{@hrd.memory[128..@hrd.program_data_pointer-1]}"
          puts "\n--------------------------------------------------------------------------"

        end

      else
        puts "HLT encountered, enter q to exit"
      end

    elsif user_input == 'a'
      puts "Executing the whole code as printed below\n\n"
      puts @hrd.memory[0...@hrd.memory_pointer]
      puts "\n--------------------------------------------------------------------------"
      while (@hrd.programCounter < @hrd.memory_pointer)       # using a while loop to execute all instructions till the last location as pointed my the memory pointer
      input_str = @hrd.memory[@hrd.programCounter]
      opcode, arg = input_str.split(' ')

        if opcode == "DEC"
          @dec.execute(arg)

        elsif opcode == "LDA"
          @lda.execute(arg)

        elsif opcode == "LDB"
          @ldb.execute(arg)

        elsif opcode == "LDI"
          @ldi.execute(arg)

        elsif opcode == "STR"
          @str.execute(arg)

        elsif opcode == "XCH"
          @xch.execute(arg)

        elsif opcode == "JMP"
          @jmp.execute(arg)

        elsif opcode== "JZS"
          @jzs.execute(arg)

        elsif opcode== "JVS"
          @jvs.execute(arg)

        elsif opcode == "ADD"
          @add.execute(arg)

        elsif opcode == "HLT"
          @hlt.execute(arg)
          puts "Zero bit = #{@hrd.zero_result_bit}, Overflow bit = #{@hrd.overflow_bit}, RegisterA = #{@hrd.register["A"]}, RegisterB = #{@hrd.register["B"]}"
          puts "Program Data"
          @hrd.symbolTable.each do |key, value|puts "#{key} = #{@hrd.memory[value]}" end
          puts "--------------------------------------------------------------------------"

        end
      end
    end
    print "\nUser Input: "
    user_input= gets.chomp
    end
  end
  end

ali = ALI.new
ali.commandLoop
