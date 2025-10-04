///
/// ARMv6 (32-bit) code generator
///

#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

// #region Basic register allocator

#define REG_NUM 4

/// @brief List of available registers and their names
static int freereg[REG_NUM];
/// @brief List of registers used by compiler
static char *reglist[REG_NUM] = {"r4", "r5", "r6", "r7"};

/// @brief Set all registers as available
void freeall_registers(void)
{
    freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

/// @brief Allocate a free register. Return the number of
/// the register. Die if no available registers.
static int alloc_register(void)
{
    for (int i = 0; i < REG_NUM; i++)
        if (freereg[i])
        {
            freereg[i] = 0;
            return i;
        }

    fatal("Out of registers");
    return NOREG; // Keep -Wall happy
}

/// @brief Return a register to the list of available registers.
/// Check to see if it's not already there.
static void free_register(int reg)
{
    if (freereg[reg] != 0)
        fatald("Error trying to free register", reg);

    freereg[reg] = 1;
}

// #endregion

// #region ASM code gen

// We have to store large integer literal values in memory.
// Keep a list of them which will be output in the postamble
#define MAXINTS 1024
/// @note Store large integer literal values which will be output
/// in the postamble following the `.L3` label.
int Intlist[MAXINTS];
static int Intslot = 0;

/// @brief Determine the offset of a large integer
/// literal from the .L3 label. If the integer
/// isn't in the list, add it.
static void set_int_offset(int val)
{
    int offset = -1;
    // See if it is already there
    for (int i = 0; i < Intslot; i++)
        if (Intlist[i] == val)
        {
            offset = 4 * i;
            break;
        }

    // Not in the list, so add it
    if (offset == -1)
    {
        offset = 4 * Intslot;
        if (Intslot == MAXINTS)
            fatal("Out of int slots in set_int_offset()");
        Intlist[Intslot++] = val;
    }

    // Load r3 with this offset
    fprintf(Outfile, "\tldr\tr3, .L3+%d\n", offset);
}

/// @brief Print out the assembly preamble
void cgpreamble()
{
    freeall_registers();
    fputs(
        "\t.text\n"              // `.text`              → begin code section
        ".LC0:\n"                // `.LC0`               → label for string literal
        "\t.string\t\"%d\\n\"\n" // `.string \"%d\\n\"`  → define string constant "%d\n"
        "printint:\n"            // function label
        "\tpush\t{fp, lr}\n"     // push old frame pointer & link register (save caller context)
        "\tadd\tfp, sp, #4\n"    // establish new frame pointer
        "\tsub\tsp, sp, #16\n"   // allocate 16 bytes stack space for locals
        "\tstr\tr0, [fp, #-8]\n" // store argument (x) at local var (fp - 8)
        "\tldr\tr1, [fp, #-8]\n" // load x into r1 (2nd printf arg)
        "\tldr\tr0, =.LC0\n"     // load address of format string into r0 (1st printf arg)
        "\tbl\tprintf\n"         // call printf("%d\\n", x)
        "\tnop\n"                // no operation (optional)
        "\tadd\tsp, fp, #0\n"    // deallocate local stack
        "\tpop\t{fp, pc}\n"      // restore fp and return to caller
        "\n",
        Outfile);
}

/// @brief Print out the assembly postamble
void cgpostamble()
{
    // #region Define global variable

    /// @note
    /// With ARM, we have to manually allocate space for all global variables in our program postamble.
    /// ```asm
    ///        .comm   i,4,4
    ///        .comm   j,1,1
    /// ...
    /// .L2:
    ///        .word i
    ///        .word j
    /// ```
    ///
    /// To access these, we need to load a register with the address of each variable,
    /// and load a second register from that address:
    /// ```asm
    ///        ldr     r3, .L2+0
    ///        ldr     r4, [r3]        # Load i
    ///        ldr     r3, .L2+4
    ///        ldr     r4, [r3]        # Load j
    /// ```
    ///
    /// Stores to variables are similar:
    /// ```asm
    ///        mov     r4, #20
    ///        ldr     r3, .L2+4
    ///        strb    r4, [r3]        # i= 20
    ///        mov     r4, #10
    ///        ldr     r3, .L2+0
    ///        str     r4, [r3]        # j= 10
    /// ```
    ///
    ///

    // Print out the global variables.
    // In other words, generate the table of `.words`
    fprintf(Outfile, ".L2:\n");
    for (int i = 0; i < Globs; i++)
        if (Gsym[i].stype == S_VARIABLE)
            fprintf(Outfile, "\t.word %s\n", Gsym[i].name);
    // #endregion

    // #region Define integer literals

    /// @note
    /// The size of an integer literal in a load instruction is limited to 11 bits.
    /// Thus, we can't put large integer literals into a single instruction.
    /// The solution is to store the literal values in memory, like variables.
    /// We keep a list of previously-used literal values and output them following the `.L3` label.

    // Print out the integer literals
    fprintf(Outfile, ".L3:\n");
    for (int i = 0; i < Intslot; i++)
        fprintf(Outfile, "\t.word %d\n", Intlist[i]);
    // #region
}

// Print out a function preamble
void cgfuncpreamble(int id)
{
    char *name = Gsym[id].name;
    fprintf(Outfile,
            "\t.text\n"
            "\t.globl\t%s\n"             // `.globl <name>`             → Declare <name> as a global symbol, visible to the linker
            "\t.type\t%s, \%%function\n" // `.type <name>, %function`   → Mark <name> as a function symbol (for debuggers/linkers)
            "%s:\n"                      // `<name>:`                   → Define the label <name> (entry point of the function)
            "\tpush\t{fp, lr}\n"         // `push  {fp, lr}`            → Save the old frame pointer and link register
            "\tadd\tfp, sp, #4\n"        // `add   fp, sp, #4`          → Add sp+4 to the stack pointer (set up the new frame pointer)
            "\tsub\tsp, sp, #8\n"        // `sub   sp, sp, #8`          → Lower the stack pointer by 8
            "\tstr\tr0, [fp, #-8]\n",    // `str   r0, [fp, #-8]`       → Store the first argument (in r0) into the local variable slot (ARM passes first arg in r0; this stores it into the local frame at offset -8 from fp).
            name, name, name);

    /// @note Stack layout after function prologue:
    ///
    ///        High addresses
    ///     ┌──────────────────────┐
    ///     │   previous FP        │  ← saved by `push {fp, lr}`
    ///     │   return address (LR)│
    ///     │──────────────────────│
    ///     │   local var (arg r0) │  ← stored at [fp, #-8]
    ///     │   local var space    │
    ///     │──────────────────────│
    ///     │                      │
    ///     │        ...           │
    ///     │                      │
    ///     └──────────────────────┘
    ///            ↓
    ///           SP (after `sub sp, sp, #8`)
}

/// @brief Print out the assembly postamble
void cgfuncpostamble(int id)
{
    cglabel(Gsym[id].endlabel); // Mark <endlabel>
    fputs(
        "\tsub\tsp, fp, #4\n" // `sub   sp, fp, #4`   → Restore stack pointer (undo local var allocation)
        "\tpop\t{fp, pc}\n"   // `pop   {fp, pc}`     → Restore previous frame pointer and return (pop into PC)
        "\t.align\t2\n"       // `.align`             → Ensures the following code or data is 4-byte aligned (ARM convention).
        "\n",
        Outfile);
}

/// @brief Load an integer literal value into a register.
/// Return the number of the register
int cgloadint(int value, int type)
{
    /// @brief r = value

    // Get a new register
    int r = alloc_register();

    if (value <= 1000)
    {
        /// @note If the literal value is small, do it with one
        /// `mov <reg>, #<value>`

        fprintf(Outfile, "\tmov\t%s, #%d\n", reglist[r], value);
    }
    else
    {
        /// @note Load the value indirectly from memory (constant pool).
        /// The helper `set_int_offset(value)` sets up the proper memory address in `r3`.
        /// ```asm
        /// ldr r3, .L3+<offset> // Load address of <value> to r3
        /// ldr r4, [r3]
        /// ```

        set_int_offset(value);
        fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
    }

    return r;
}

/// @brief Determine the offset of a variable from the `.L2`
/// label. Yes, this is inefficient code.
static void set_var_offset(int id)
{
    int offset = 0;
    // Walk the symbol table up to id.
    // Find S_VARIABLEs and add on 4 until
    // we get to our variable

    for (int i = 0; i < id; i++)
        if (Gsym[i].stype == S_VARIABLE)
            offset += 4;

    /// @note
    /// As explain in function `cgpostamble`, in ARM, to access or store value
    /// to a global variable, we must load a register with the address of this
    /// variable first.
    /// Below code is for this purpose. It emits `ldr r3, .L2+<offset>`, then
    /// we can load value from variable like `ldr r4, [r3]` or set value to
    /// that variable like `strb r4, [r3]`

    // Load r3 with this offset
    fprintf(Outfile, "\tldr\tr3, .L2+%d\n", offset);
}

/// @brief Load a value from a variable into a register.
/// Return the number of the register
int cgloadglob(int id)
{
    // Get a new register
    int r = alloc_register();

    /// @note
    /// ```asm
    /// ldr     r3, .L2+<offset>       # Load addr of i into register r3 (emit by `set_var_offset()`)
    /// ldr     r4, [r3]               # Load i
    /// ```
    ///

    // Get the offset to the variable
    set_var_offset(id);
    fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
    return r;
}

/// @brief Add two registers together and return
/// the number of the register with the result
int cgadd(int r1, int r2)
{
    /// @brief r2 = r2 + r1
    ///
    /// @note `add <r2>, <r1>, <r2>`

    fprintf(Outfile, "\tadd\t%s, %s, %s\n", reglist[r2], reglist[r1], reglist[r2]);
    free_register(r1);
    return r2;
}

/// @brief Subtract the second register from the first and
/// return the number of the register with the result
int cgsub(int r1, int r2)
{
    /// @brief r1 = r1 - r2
    ///
    /// @note `sub <r1>, <r1>, <r2>`

    fprintf(Outfile, "\tsub\t%s, %s, %s\n", reglist[r1], reglist[r1], reglist[r2]);
    free_register(r2);
    return r1;
}

/// @brief Multiply two registers together and return
/// the number of the register with the result
int cgmul(int r1, int r2)
{
    /// @brief r2 = r2 * r1
    ///
    /// @note `mul <r2>, <r1>, <r2>`

    fprintf(Outfile, "\tmul\t%s, %s, %s\n", reglist[r2], reglist[r1], reglist[r2]);
    free_register(r1);
    return r2;
}

/// @brief Divide the first register by the second and
/// return the number of the register with the result
int cgdiv(int r1, int r2)
{
    /// @brief r1 = r1 / r2
    ///
    /// @note To do a divide: r0 holds the dividend, r1 holds the divisor.
    /// The quotient is in r0.
    /// ```asm
    /// mov   r0, <r1>         // Move dividend into r0 (per AAPCS calling convention)
    /// mov   r1, <r2>         // Move divisor into r1
    /// bl    __aeabi_idiv     // Call runtime helper: signed division
    /// mov   <r1>, r0         // Move quotient result from r0 back into destination register
    /// ```
    /// `__aeabi_idiv` is a runtime helper defined by the ARM EABI for 32-bit signed division.
    ///

    fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r1]);
    fprintf(Outfile, "\tmov\tr1, %s\n", reglist[r2]);
    fprintf(Outfile, "\tbl\t__aeabi_idiv\n");
    fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

/// @brief printint() with the given register
void cgprintint(int r)
{
    /// @note
    /// /// According to the ARM Procedure Call Standard (AAPCS):
    /// - The first argument to a function is passed in `r0`.
    ///
    /// ```asm
    /// mov   r0, <rN>      // Move the value to r0 (function argument)
    /// bl    printint      // Branch with link to printint()
    /// nop                 // No operation (padding or alignment)
    /// ```

    fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
    fprintf(Outfile, "\tbl\tprintint\n");
    fprintf(Outfile, "\tnop\n");
    free_register(r);
}

/// @brief Call a function with one argument from the given register
/// Return the register with the result
int cgcall(int r, int id)
{
    /// @note
    /// ```asm
    /// mov   r0, <arg_reg>     // Move the argument into r0 (1st argument)
    /// bl    <func_name>       // Branch with link (call function)
    /// mov   <dest_reg>, r0    // Copy return value from r0 into a new register
    /// ```

    fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
    fprintf(Outfile, "\tbl\t%s\n", Gsym[id].name);
    fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r]);
    return r;
}

/// @brief Store a register's value into a variable
int cgstorglob(int r, int id)
{
    /// @brief identifier = r
    ///
    /// @note
    ///
    /// __For P_CHAR__
    /// `strb <r>, [r3]` → Store a single byte (8 bits) from register `<r>`
    ///                     into memory at address `[r3]`.
    ///
    /// __For P_INT__
    /// `str  <r>, [r3]` → Store a word (32 bits) from register `<r>`
    ///                     into memory at address `[r3]`.
    ///
    /// __For P_LONG__
    /// `str  <r>, [r3]` → Store a double word (32 bits) from register `<r>`
    ///                     into memory at address `[r3]`.
    ///
    /// The address `[r3]` is set earlier by `set_var_offset(id)`, which loads
    /// the address of the variable into register `r3`.
    ///

    // Get the offset to the variable
    set_var_offset(id);

    switch (Gsym[id].type)
    {
    case P_CHAR:
        fprintf(Outfile, "\tstrb\t%s, [r3]\n", reglist[r]);
        break;
    case P_INT:
    case P_LONG:
        fprintf(Outfile, "\tstr\t%s, [r3]\n", reglist[r]);
        break;
    default:
        fatald("Bad type in cgloadglob:", Gsym[id].type);
    }

    return r;
}

/// @brief Array of type sizes in P_XXX order.
/// 0 means no size.
static int psize[] = {
    [P_NONE] = 0,
    [P_VOID] = 0,
    [P_CHAR] = 1,
    [P_INT] = 4,
    [P_LONG] = 4};

/// @brief Given a P_XXX type value, return the
/// size of a primitive type in bytes.
int cgprimsize(int type)
{
    // Check the type is valid
    if (type < P_NONE || type > P_LONG)
        fatal("Bad type in cgprimsize()");
    return psize[type];
}

/// @brief Generate a global symbol
void cgglobsym(int id)
{
    /// @note
    ///
    /// __For P_INT__
    /// `.comm sym, 4, 4` → Tells the assembler/linker to reserve uninitialized storage
    ///                           (like a global variable in C without an initializer).
    ///
    /// `x` → the symbol name (global variable name).
    /// `4` → allocate 4 bytes (size of a 32-bit integer).
    /// `4` → align it on an 4-byte boundary.
    ///
    /// This is exactly what GCC/Clang do for code like `int x;`.
    ///
    /// __For P_CHAR__
    /// `.comm sym, 1, 1`
    ///
    /// __For P_LONG__
    /// `.com sym, 4, 4`
    ///
    /// @details `.comm symbol, length, alignment`
    ///

    // Get the size of the type
    int typesize = genprimsize(Gsym[id].type);

    fprintf(Outfile, "\t.comm\t%s,%d,%d\n", Gsym[id].name, typesize, typesize);
}

// List of comparison instruction
static char *cmplist[] = {
    "moveq", // A_EQ
    "movne", // A_NE
    "movlt", // A_LT
    "movgt", // A_GT
    "movle", // A_LE
    "movge", // A_GE
};

/// @brief List of inverted comparison instructions
static char *invcmplist[] = {
    "movne", // A_EQ
    "moveq", // A_NE
    "movge", // A_LT
    "movle", // A_GT
    "movgt", // A_LE
    "movlt", // A_GE
};

/// @brief Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    /// @note
    /// ```asm
    /// cmp   <r2>, <r1>                // Compare <r1> and <r2> (sets condition flags)
    /// <cond> <r2>, <r2>, #1           // If condition true, set r2 = 1
    /// <invcond> <r2>, <r2>, #0        // If condition false, set r2 = 0
    /// uxtb  <r2>, <r2>                // Zero-extend the result to 8 bits (ensure 0 or 1)
    /// ```
    /// Explanation:
    /// - `cmp` sets flags (N, Z, C, V) based on `<r2> - <r1>`.
    /// - Conditional move instructions (`moveq`, `movne`, `movlt`, etc.)
    ///   use these flags to assign 1 or 0 to the result register.
    /// - `uxtb` ensures the value is cleanly 0 or 1 in the lower byte.
    ///

    fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
    fprintf(Outfile, "\t%s\t%s, #1\n", cmplist[ASTop - A_EQ], reglist[r2]);
    fprintf(Outfile, "\t%s\t%s, #0\n", invcmplist[ASTop - A_EQ], reglist[r2]);
    fprintf(Outfile, "\tuxtb\t%s, %s\n", reglist[r2], reglist[r2]);
    free_register(r1);
    return r2;
}

/// @brief Generate a label
void cglabel(int l)
{
    /// @note `L<label>:` - Emit an assembly label definition

    fprintf(Outfile, "L%d:\n", l);
}

/// @brief Generate a jump to a label
void cgjump(int l)
{
    /// @note `b L<label>` - Emit an unconditional jump to a specified label.

    fprintf(Outfile, "\tb\tL%d\n", l);
}

/// @brief List of inverted branch instructions
static char *brlist[] = {
    "bne", // A_EQ
    "beq", // A_NE
    "bge", // A_LT
    "ble", // A_GT
    "bgt", // A_LE
    "blt", // A_GE
};

/// @brief Compare two registers and jump if false
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    /// @note
    /// ```asm
    /// cmp   <r1>, <r2>        // Compare r1 - r2 (sets N, Z, C, V flags)
    /// b<cond> L<label>        // Branch to label if condition satisfied
    /// ```
    ///
    /// Explanation:
    /// - `cmp` updates the condition flags (Negative, Zero, Carry, Overflow).
    /// - `b<cond>` performs a branch based on these flags.
    /// - The condition suffix `<cond>` comes from `brlist[ASTop - A_EQ]`.
    ///

    fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
    fprintf(Outfile, "\t%s\tL%d\n", brlist[ASTop - A_EQ], label);
    freeall_registers();

    return NOREG;
}

/// @brief Widen the value in the register from the old
/// to the new type, and return a register with
/// this new value
int cgwiden(int r, int oldtype, int newtype)
{
    // Nothing to do
    return r;
}

// Generate code to return a value from a function
void cgreturn(int reg, int id)
{
    /// @note
    /// ```asm
    /// mov   r0, r2      // Move return value into r0
    /// b     Lend        // Jump to function epilogue
    /// ```
    ///
    /// ARM Procedure Call Standard (AAPCS):
    /// - The function return value is always placed in `r0`.
    /// - For 64-bit values (`long long` or pointer-sized on AArch64), `r0`–`r1` may be used.
    /// - The callee ensures that the stack and frame are restored before returning.
    ///

    fprintf(Outfile, "\tmov\tr0, %s\n", reglist[reg]);

    /// @note
    /// After placing the return value in `r0`, unconditionally branch to the function’s
    /// epilogue label (`L<endlabel>`), where stack cleanup and `ret` occur.
    ///

    cgjump(Gsym[id].endlabel);
}

// #endregion