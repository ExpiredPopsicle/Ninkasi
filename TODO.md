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
