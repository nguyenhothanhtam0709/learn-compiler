# Note about some issue

### Invoke variadic function on AArch64

On AArch64 macOS only the fixed arguments (like the format string in printf) go in registers, all variadic arguments are passed on the stack, not in registers, even if there are fewer than eight arguments in total.  
For example, On macOS ARM64, \_printf is implemented as a variadic function, so:

- The first argument (`x0`) = format string → in register.
- Second argument (the `42` for `%d`) = variadic → passed on the stack, not x1, because `%d` is variadic.

```asm
    .global _main
    .extern _printf

    .section __TEXT,__cstring
fmt:
    .asciz "%d\n"

    .text
_main:
    stp     x29, x30, [sp, -16]!   // save frame pointer and link register
    mov     x29, sp
	sub		sp, sp, #32 // Push 2nd argument of printf into top of caller stack

	mov 	w0, #42
	str		w0, [x29, #-32]

    adrp    x0, fmt@PAGE            // x0 = address of format string (1st argument)
    add     x0, x0, fmt@PAGEOFF
    bl      _printf                 // call printf("%d\n", 42)

    // exit(0)
	add     sp, sp, #32
    ldp     x29, x30, [sp], 16
    ret
```
