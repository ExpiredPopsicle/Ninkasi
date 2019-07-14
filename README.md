
```
   ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
  •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
  ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
  ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
  ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
```

----------------------------------------------------------------------

Ninkasi
=======

Ninkasi is yet another scripting language, targeting primarily games.

Quick Tutorial
==============

Simplest hosting application API example
----------------------------------------

TODO

Documentation
=============

Installing/Compiling
--------------------

Ninkasi is a standard Autotools-based project. Run the following from
the command line to compile and install.

If you're using the source straight from the Git repository, go to the
project directory and run:

```
./autogen.sh && ./configure && make && make install
```

If you're using a tarball version, you can skip the autogen.sh and
run:

```
./configure && make && make install
```

Ninkasi has no library dependencies besides the standard C library
(C89 or ANSI C). If you have trouble compiling, please submit a bug
report.

The Language Itself
-------------------

### Comments

Comments are done using C++-style double forward slashes.

```
// This is a comment.
```

### Variable declarations

Non-function variables are declared with the "var" keyword.

```
var foo;
```

Variables may have an initial value and type when declared.

```
var foo = 1234;
```

If variables are not given an initial value, they will default to an
integer 0.

Ninkasi uses dynamic typing. The basic data types are:

1. Integers - Signed 32-bit integers.
2. Float - 32-bit floating point values. Anything with a decimal point
   will be read in as a float.
3. String - UTF-8 strings. Cannot contain '0' bytes due to C string
   limitations.
4. Function ID - A reference to a function. This variable can be
   called to execute that function. (See Function declarations later
   in this document.)
5. Object ID - A pointer to an object. (See Objects later in this
   document.)
6. Nil

### "if" statements

"if" statements are constructed like in C.

```
if(foo) {
    // Do something.
}
```

As with C, non-zero values indicate true, while zero indicates false.
Non-integer values will be converted to integers for this test, so the
following would not pass:

```
if("0") {
    // Do something.
}
```

"if" statements may omit the curly brace block if the code to execute
is only a single statement.

```
if(foo)
    print("foo is true!");
```

"if" statements may include "else" code, which will execute when the
test does not pass.

```
if(0) {
    print("This code will never happen.");
} else {
    print("This code will always happen.");
}
```

Note that "if" statements do not support C-style shortcutting. If an
early exit from the test expression is needed, it must be split into
multiple nested "if" statements.

### "while" and "for" loops

"while" loops are constructed like in C, and have the same semantics
for evaluating the expression as the "if" statements.

```
while(foo) {
    print("This is executing in a loop.\n");
}
```

"for" loops are constructed similarly to how they are in C.

```
var foo;
for(foo = 0; foo < 10; ++foo) {
    print("Still looping: ", foo, "\n");
}
```

"for" loops may include a variable declaration inside the statement.
That variable will be in-scope for the body of the loop.

```
for(var foo = 0; foo < 10; ++foo) {
    print("Still looping: ", foo, "\n");
}
```

### Function declarations

Functions are declared with the "function" keyword.

```
function somestuff()
{
    print("Inside a function.\n");
}
```

This will create a variable named "somestuff" with a type of function
ID. That variable will be a reference to this function. The variable
will exist in the scope of both the parent context AND the function
context itself.

Function arguments may be added by adding their names with a
comma-separated list within the parentheses.

```
function somestuff(x, y, z)
{
    print(x + y + z, "\n");
    print("Inside a function.\n");
}
```

Functions may return a value with the "return" statement. Execution of
this statement will stop execution of the function and return control
to the caller.

```
function somestuff(x, y, z)
{
    return x + y + z;
}
```

Functions that do not have an explicit return statement will return an
integer 0.

#### Anonymous functions

Functions may be declared without a name. When this syntax is used,
the function declaration is considered an expression instead of an
entire statement, the return value of which is the function object.

```
// Declare a function and assign it to a variable immediately.
// Effectively the same as declaring it with the name. Note that the
// semicolon is needed here to make this a complete statement, where it
// is not needed in a named function declaration.
var foo = function(a, b, c)
{
    return x + y + z;
};

// Declare an anonymous function and call it immediately.
function(a) { print(a); }("This is a test.");
```

### Function calls

Functions are called with the parentheses operator.

```
somestuff(1, 2, 3);
```

Function calls will evaluate to the return value.

```
function somestuff(x, y, z)
{
    return x + y + z;
}

var foo;
foo = somestuff(1, 2, 3);
print("Foo: ", foo, "\n");
```

The output of this program is:

```
Foo: 6
```

Function calls are not required to use the variable name assigned to
it originally.

```
function func1()
{
    print("In func1\n");
}

function func2()
{
    print("In func2\n");
    return func1;
}

func2()();
```

The output of this program is:

```
In func2
In func1
```

### Objects

Object are hash tables which are stored in a pool that is regularly
garbage-collected. The keys and values for this hash table can be any
of the basic types.

Objects can be created with the "newobject" keyword.

```
var foo = newobject;
```

The contents of objects can be accessed with the brackets operator, or
the dot ('.').

```
var foo = newobject;
var junk = 45;
foo["asdf"] = 1234;
foo[junk] = 5678;
print(foo[junk], "\n");
print(foo.asdf, "\n");
```

Output:

```
5678
1234
```

Note that the dot only works with strings coded directly into the
script, where the brackets can contain any valid expression.

There are no built-in functions to get the number of key/value pairs
or iterate over them. These must be added as part of a library.
(FIXME.)

What the API Prefixes Mean
----------------------

1. nki* = Ninkasi internal
  - Called from inside VM code.
  - Not called from outside VM code.
    - Calling these directly instead of going through the nkx-prefixed
      functions will result in error handling code being skipped.

2. nkx* = Ninkasi external
  - Wrappers around internal functions that set up error handlers
    before calling the wrapped function and cleaning them up before
    returning.
  - Called from outside the VM, by the host application.
  - NOT called from inside the VM.
    - Calling from inside the VM will cause the error handlers to be
      setup twice.

Error handling
--------------

### Normal error handling

In the event of a "normal" error, the VM's state can be considered
valid in that there will be no dangling pointers, corrupted memory, or
null pointers for things that would not normally have a null pointer
as part of the normal program operation.

The state may be inspected through many of the public API functions,
but attempting to resume operation on the program running in the VM
will probably not succeed. (And even if it did, the program will
probably have some unexpected state as a result of the error.)

TODO: Example code showing how to detect and deal with an error.

### Catastrophic error handling

"Catastrophic" errors are generally out-of-memory conditions. The
hosting application can set a limit on the amount of memory the VM is
allowed to allocate (FIXME: currently set in
vm->limits.maxAllocatedMemory). When this memory OR the hosting
application address space is exhausted, an error handler will be
initiated. This sets a flag on the VM and returns (via longjmp) to the
last nkx* function, which then returns back to the hosting
application. Any further calls to nkx* functions will return without
doing anything.

Because the error handling for this is at such a low level, the VM
state may be invalid or incomplete when control is returned to the
hosting application. The allocation list inside the VM is handled at
the same level as these errors, and should always be consistent.
Cleaning up the VM with nkxVmDelete() will deallocate the VM and all
of the VM's own allocations, but no other interaction with the VM
should happen.

- Do not trust any pointer inside the VM to be a valid pointer.
- The allocation list is the only set of guaranteed valid pointers.
- The VM may be safely deleted with nkxVmDelete().

TODO: Example code showing how to detect and deal with a catastrophic
error.

Sandboxing
----------

TODO

FAQ
===

1. Why the hell would I use this instead of Lua?
  * You probably shouldn't, but because you asked: No stupid one-based
    indexing. Superior semicolons and curly braces instead of blocks
    that end in weird ways depending on what started them. The ability
    to (relatively) trivially serialize and deserialize the entire VM
    state (TODO). Easy sandboxing and hosting-application-defined
    memory usage limits.

2. Why did you make this when so many other better languages exist?
  * Seemed like a good idea at the time. May have also been a bit
    drunk.

3. Why the DOS compatibility stuff?
  * I was making a DOS game.

4. Is it fast?
  * Not particularly. Limited speed tests put it at about 1/10th the
    speed of Lua for an extremely simple program.

5. Are you drunk right now?
  * Almost certainly.

