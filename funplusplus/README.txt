ABOUT PROJECT
This project is an improvement on the fun language. We've added several different types of 
data structures, a string type, and generally improved language constructs. Fun++ can support
sorting algorithms, data structures algorithms, and storage of large amounts of data.
We had lots of fun working on Fun++!

PROJECT SYNTAX 
GENERAL NOTES
<type> always refers to the options "int" and "string"

CREATING VARIABLES
x = <type> <value>

ARRAYS
x = array <type> <# of elements> <comma separated elements>
x[<index>] (this returns the element at the given index)

ARRAYLISTS
x = arraylist <type> <# of elements> <comma separated elements>
x[<index>] (like array indexing)
x insert <item>
x remove <index>

LINKEDLISTS
x = linkedlist <type> <# of elements> <comma separated elements>
x insert <item>
x remove <index>

QUEUES
x = queue <type> <first element>
x add <item>
x remove <item>
x peek (this returns the first added element without removing it)

OPERATORS
- Subtraction is supported by the minus sign
- Comparison is supported by > and <

ERROR MESSAGES
1. Out of Bounds index checking
2. Syntax errors

STATIC TYPE CHECKING
- Fun++ is a static strong language
- Data structures and string data types are strong and static
    - Variables cannot be redefined as different types
    - Values added to data structures cannot have a different type from the data structure
- Function arguments have yet to be added, so they are not strong and statically typed yet
- Static type checking is done before interpreting, and it gives error messages for type
    and syntax errors

FUNCTION ARGUMENTS
x = fun (<type> parameter... (must be an ID variable))
- not statically and strongly typed yet
- supports basic parameter types including data structures, ints, and strings

STRING RULES
- x = "string"
- see t_string.fun
- String data type is not allowed in expressions such as "hello" + "world"

FUTURE WORK
- Simple Objects are still in progress on a different branch, but we had difficulty
    implementing them due to a bug with copying the trie
- Static type check function arguments
- We would have liked to add more test cases to show off the awesome functionality of Fun++!

