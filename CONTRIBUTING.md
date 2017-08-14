Contributing
------------

All copyright must be assigned to Clifford Jolly.

Style Guide
-----------

All internal (non-public-API) functions must start with "nki".

All external (public API) functions must start with "nkx".

All integers and booleans must be of one of the types from nktypes.h.

All code must compile on a C89 compiler.

All 8-bit strings should be considered to be UTF-8 strings.

All headers must use #ifdef include guards.

All #defines for include guards must start with "NINKASI_".

All #endifs for include guards must have a comment indicating what
#ifdef they are finishing.

All comments must use the C++ style double-forward-slash style ("//"),
unless there is a good reason not to.

All filenames must be lowercase on case-sensitive operating systems.
All #include directives that refer to other source files must use
lowercase.

All filenames must have a maximum of eight characters for a name, and
three characters for an extension, for DOS compatibility.

All source filenames must start with "nk".

All code must run under Valgrind with zero errors.

Triple-forward-slash comments should be use used for documentation of
API functions that are visible outside of the scope of a single C
file, and should immdiately precede the function's declaration in the
header file it exists in.

Code must not produce any compiler warnings at these levels...

* Clang: Using the "-Wall" command line parameter.

* GCC: Using the "-Wall" command line parameter.

* Watcom: Standard level of verbosity.

Curly braces surrounding the body of a function or structure should
start on the next line.

```c
void nkiFoo(void)
{
}
```

Curly braces for flow control within a function should start one the
same line.

```c
if(foo) {
}
```

Curly braces for flow control within a function, where the loop
conditional or init code preceding the block extends past a single
line should start on the next line.

```c
if(very long
   conditional that
   does not fit on a
   single line)
{
}
```

All C functions that take no parameters must use "void" in the
parameter list.

