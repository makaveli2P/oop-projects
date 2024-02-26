# Author  :   Dhruv Agarwal (UIN: 651945114)
# Name    :   dhruv_agarwal.rb
# Date    :   October 2, 2022
# Content :   A Simple Set Calculator (using binary search trees)

class Node
 # three instance variables, left, right and values.
 attr_reader :values                          # values with a getter but no setter
 attr_accessor :left, :right, :root           # left and right with both getter and setter; this also acts as the instance variable

 def initialize(values)                # Node is initialized with 'values'
  @values = values                           # storing the values in an instance variable of the same name
 end
end

class BST
 # instance variable root with both getter and setter
 attr_accessor :root, :set_X, :set_Y, :set_Z

 def initialize
  @root = nil                           # @root initialized to nil and not 0 to avoid BST having a root with 0 value
 end

 # method to insert value in the BST while preserving BST property
 def insert_tree(node, head = @root)                    # assigned @root to head because formal argument cannot be an instance variable

  return @root = node if @root.nil?
  return node if head.nil?                             # handling nil values for head

  if node.values < head.values
   head.left = insert_tree(node, head.left)           # adding the node as left child if the value is less than the head
  elsif node.values > head.values
   head.right = insert_tree(node, head.right)         # adding the node as right child if the value is greater than the head
  end

  head                                                 #returning the head of the tree
 end

 # method for in-order traversal of the constructed BST (left -> root -> right)
 def in_order(node = self.root)

  if node != nil
   in_order(node.left)       # recursively checking left values of a node first, in accordance with inorder traversal.
   puts node.values          # printing the values of the node
   in_order(node.right)      # recursively checking right values of a node last, in accordance with inorder traversal.
  end

 end

 # method to traverse the BST when instantiated
 def traverse(user_input)

  tree = BST.new
  user_input.each { |i| tree.insert_tree(Node.new(i)) }      # each value of the user input numeric arguments getting inserted to the BST instantiated above
  tree.in_order
  tree

 end

 def &(set)              # redefining the intersection & operator to work on BST class objects
  @root & set.root
 end

 def |(set)              # redefining the union | operator to work on BST class objects
  @root | set.root
 end

end

# print "Enter the first set X: "
# set_X = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set X
#
# p set_X.class
# print "Enter the second set Y: "
# set_Y = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set Y
#
# print "Enter the third set Z: "
# set_Z = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set Z
#
# puts "------------------------------------------------------"

class CommandMenu

 def initialize(*menu_commands)            # initialize with an instance variable menu_command
  @menu_commands = menu_commands                # storing the values in an instance variable of the same name
 end

 def input_command
  @menu_commands.each_with_index do|item, index|
   puts "#{index + 1} for #{item}"             # printing the menu with a numeric ordering according to their indexes
  end

  print "Enter your command (1,2,3,4,5,6,7,8,9,10,11): "
  user_command = gets.to_i                      # input to get the user desired command
  return user_command
 end

end

menu = CommandMenu.new("X: Values","Y: Values","Z: Values","a: insert","r: rotate","s: switch","u: union","i: intersection","c: deep copy","l: lambda","q: quit")

while command = menu.input_command
 case command

 when 1
 print "Enter the first set X: "
 set_X = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set X
 p set_X.class

 when 2
 print "Enter the second set Y: "
 set_Y = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set Y

 when 3
 print "Enter the third set Z: "
 set_Z = BST.new.traverse(gets.chomp.split(',').map(&:to_i))       # takes a comma-separated list of numeric arguments for set Z

 when 4                                                    # i — numeric argument i is inserted in an appropriate location in set X
  print "Enter a value to be inserted in set X: "
  input_i = gets.to_i
  set_X.insert_tree(Node.new(input_i))                    # insert i into already constructed BST X by invoking insert_tree on it

 when 5                                                    # the content of the 3 sets is rotated
  set_Y,set_Z,set_X = set_X,set_Y,set_Z                   # parallel assignment of Y to X, Z to Y and X to Z (RUBY IS AMAZING!!)

 when 6                                                    # the contents of sets X and Y are switched
  set_X,set_Y = set_Y,set_X                               # parallel assignment of X to Y and Y to X

 when 7                                                    # this command takes the union of sets X and Y. the result set is stored as X.
  set_X.in_order | set_Y.in_order

 when 8                                                    # this command takes the intersection of sets X and Y. the result set is stored as X.
  set_X.in_order & set_Y.in_order

 when 9                                                    # set X is recursively deep copied into set Y
  set_Y = Marshal.load(Marshal.dump(set_X))

 when 10                                                    # this command takes a string argument defining a one-argument lambda expression
  aString = "-> (value){value + 2}"                       # this lambda is applied to every element of set X
  aBlock = eval aString
  aBlock.call(set_X.in_order)

 when 11                                                    # this command quits the execution
  break
 end

 puts "------------------------------------------------------"
 puts "Value of set X after this command is : "
 set_X.in_order                                            # to print the contents of set X after every command execution
 puts "Value of set Y after this command is : "
 set_Y.in_order                                            # to print the contents of set Y after every command execution
 puts "Value of set Z after this command is : "
 set_Z.in_order                                            # to print the contents of set Z after every command execution
 puts "------------------------------------------------------"
end