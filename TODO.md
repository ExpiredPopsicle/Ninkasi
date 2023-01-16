# Documentation
- Better simple example programs.
- How do I... ?
  - Create an object
  - Return a value
  - Setup a C function
- Sandboxing.
- Garbage collection gotchas.

# Utils library
- Move stuff in from libdanmaku for some optional built-in functions.
  - Nice to have: rand (RC4), sin, cos, abs, float, int
  - Important:    len, array

# General API cleanup.
- nkxVm vs just nkx - seems arbitrary.
- Simplify external function creation.
- Add functions to simplify setting object key/values.
  - Should be easier to just set or lookup string and number keys.

# C++ API?
- Operator overloading for easy NKValue manipulation is difficult
  without extra tracking of which objects go to which VM.

# Stream 2022-12-18
- Switch internal string format from to null-terminated to just storing length.
  - We'll still have to keep the null terminator, or at least room for it, like std::string.
- String encoding/decoding.
- String indexing.
- Start peeling away setjmp/longjmp use.
- Test harness cleanup.
- TODOs cleanup.
- Compiler warning system.
- Check for examples of external-to-internal calls.

# Stream 2022-01-15
- Modify test harness to use the FIXED file loading code from interp.
- FSE for is_coroutine_finished.

# Done stuff
x Create actual VM functions to handle all the weird stuff we did in
  the interpreter for error handling in the REPL.
x Make malloc failure in REPL not-resumable.
x Figure out what the heck to do with coroutine execution context when an error occurs.
  n Take coroutine objects and turn them into NILs for everything in
    the execution context stack?
x Restore old compile flags.
x Disable extra fancy leak tracking.
x Verbosity settings for test harness.
x Command line instruction count limit for test harness.


