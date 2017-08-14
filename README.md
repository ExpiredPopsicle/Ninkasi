
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
--------------

TODO

Sandboxing
----------

TODO

Installing/Compiling
--------------------

TODO

Documentation
=============

What the Prefixes Mean
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

### Normal errors

In the event of a "normal" error, the VM's state can be considered
valid in that there will be no dangling pointers, corrupted memory, or
null pointers for things that would not normally have a null pointer
as part of the normal program operation.

The state may be inspected through many of the public API functions,
but attempting to resume operation on the program running in the VM
will probably not succeed. (And even if it did, the program will
probably have some unexpected state as a result of the error.)

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

### Detecting catastrophic failures

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

