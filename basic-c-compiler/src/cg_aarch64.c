///
/// AArch64 code generator for MacOS (Apple Silicon)
///

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defs.h"
#include "data.h"
#include "decl.h"
#include "compat.h"

// #region Basic register allocator

#define REG_NUM 4

/// @brief List of available registers and their names
static int freereg[REG_NUM];
/// @brief List of registers used by compiler
static char *reglist[REG_NUM] = {"x9", "x10", "x11", "x12"};
/// @brief List of registers that are lower 32-bit version of each register of `reglist`
static char *dreglist[REG_NUM] = {"w9", "w10", "w11", "w12"};

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
static struct
{
    int val;
    int l;
} Intlist[MAXINTS];
static int Intslot = 0;

/// @brief Determine the offset of a large integer
/// literal from the .L3 label. If the integer
/// isn't in the list, add it.
static int get_int_label(int val)
{
    for (int i = 0; i < Intslot; i++)
        if (Intlist[i].val == val)
            return Intlist[i].l;

    if (Intslot == MAXINTS)
        fatal("Out of int slots in get_int_label()");

    int idx = Intslot++;
    Intlist[idx].val = val;
    Intlist[idx].l = genlabel();
    return Intlist[idx].l;
}

static int load_int_label(int val)
{
    int lidx = get_int_label(val);

    fprintf(Outfile,
            "\tadrp\tx3, __intconst_%d@PAGE\n"
            "\tadd\tx3, x3, __intconst_%d@PAGEOFF\n",
            lidx, lidx);
}

/// @brief Print out the assembly preamble
void cgpreamble()
{
    freeall_registers();
    fputs(
        "\t.extern _printf\n" // `.extern _printf`
        "\n"
        "\t.cstring\n"
        "msgfmt:\n"
        "\t.asciz \"%d\\n\"\n"
        "\n"
        // #region Define function `printint`
        "\t.text\n"                       // `.text`              → begin code section
        "\t.global printint\n"            // `.global printint`
        "printint:\n"                     // `printint:`
        "\tstp\tx29, x30, [sp, -16]!\n"   // `stp     x29, x30, [sp, -16]!` push FP (x29) and LR (x30); update SP
        "\tmov\tx29, sp\n"                // `mov     x29, sp` set new frame pointer
        "\tsub\tsp, sp, #16\n"            // `sub     sp, sp, #16` allocate 16 bytes stack space for locals
        "\tstr\tw0, [sp, #0]\n"           // `str     w0, [sp, #0]` store argument (x) at local var (fp - 16)
        "\tadrp\tx0, msgfmt@PAGE\n"       // `adrp    x0, msgfmt@PAGE ` load high part of address of format string
        "\tadd\tx0, x0, msgfmt@PAGEOFF\n" // `add     x0, x0, msgfmt@PAGEOFF` add low 12-bit offset → x0 = address of .LC0
        "\tldr\tw1, [sp, #0]\n"           // `ldr     w1, [sp, #0]`
        "\tbl\t_printf\n"                 // `bl      _printf` call printf("%d\n", x), note: on macOS, symbols from libc have leading underscore
        "\tadd\tsp, sp, #16\n"            // `add     sp, sp, #16` deallocate local stack
        "\tldp\tx29, x30, [sp], 16\n"     // `ldp     x29, x30, [sp], 16` restore FP & LR; adjust SP back
        "\tret\n"                         // `ret` return to caller
        // #endregion
        "\n",
        Outfile);
}

/// @brief Print out the assembly postamble
void cgpostamble()
{
    // #region Define global variable

    /// @note
    /// With AArch64, we have to manually allocate space for all global variables in our program postamble.
    /// ```asm
    ///        .section __DATA,__bss
    /// .global uninit1
    /// .align 1
    /// uninit1:
    ///    .skip 1          // 1-byte uninitialized
    /// ```
    ///
    /// To access these, we need to load a register with the address of each variable,
    /// and load a second register from that address:
    /// ```asm
    ///     adrp x3, uninit1@PAGE   // load page address
    ///     add  x3, x3, uninit1@PAGEOFF  // full address
    ///     ldr  x0, [x3]             // load value
    /// ```
    ///

    // Print out the global variables.
    // fprintf(Outfile, "\t.section __DATA,__bss\n");
    // // fprintf(Outfile, "\t.section __DATA,__data\n");
    // for (int i = 0; i < Globs; i++)
    //     if (Gsym[i].stype == S_VARIABLE)
    //     {
    //         char *name = Gsym[i].name;
    //         int typesize = genprimsize(Gsym[i].type);
    //         int align = log2(typesize);

    //         fprintf(Outfile,
    //                 ".global __global_%s\n"
    //                 ".align %d\n"
    //                 "__global_%s:\n"
    //                 "\t.skip %d\n"
    //                 "\n",
    //                 name, align, name, typesize);

    //         /// @note Define global variable like below to ensure the adjacency

    //         // fprintf(Outfile,
    //         //         ".globl __global_%s\n"
    //         //         "__global_%s:\n",
    //         //         name, name);
    //         // switch (typesize)
    //         // {
    //         // case 1:
    //         //     fprintf(Outfile, "\t.byte\t0\n");
    //         //     break;
    //         // case 4:
    //         //     fprintf(Outfile, "\t.long\t0\n");
    //         //     break;
    //         // case 8:
    //         //     fprintf(Outfile, "\t.quad\t0\n");
    //         //     break;
    //         // default:
    //         //     fatald("Unknown typesize in cgglobsym: ", typesize);
    //         // }
    //         putc('\n', Outfile);
    //     }
    // fputs("\n", Outfile);
    // #endregion

    // #region Define integer literals

    /// @note
    /// The size of an integer literal in a load instruction is limited to 16 bits.
    /// Thus, we can't put large integer literals into a single instruction.
    /// The solution is to store the literal values in memory, like variables.
    /// We keep a list of previously-used literal values.

    // Print out the integer literals
    fprintf(Outfile, "\t.section __TEXT,__const\n");
    for (int i = 0; i < Intslot; i++)
    {
        int lidx = Intlist[i].l;
        fprintf(Outfile,
                ".global __intconst_%d\n"
                "__intconst_%d:\n"
                "\t.quad %d",
                lidx, lidx, Intlist[i].val);
    }
    fputs("\n", Outfile);
    // #region

    // #region Print out the string literals
    // fputs(
    //     "\n"
    //     "\t.section __TEXT,__cstring\n"
    //     "msgfmt:\n"
    //     "\t.asciz \"%d\\n\"",
    //     Outfile);
    // #endregion
}

// Print out a function preamble
void cgfuncpreamble(int id)
{
    char *name = Gsym[id].name;
    if (!strcmp(name, "main"))
        name = strdup("_main");
    fprintf(Outfile,
            "\t.text\n"
            "\t.global\t%s\n"               // `.global <name>`             → Declare <name> as a global symbol, visible to the linker
            "%s:\n"                         // `<name>:`                   → Define the label <name> (entry point of the function)
            "\tstp\tx29, x30, [sp, -16]!\n" // `stp     x29, x30, [sp, -16]!` push FP (x29) and LR (x30); update SP
            "\tmov\tx29, sp\n",             // `mov     x29, sp` set new frame pointer
            // "\tadd\tfp, sp, #4\n"           // `add   fp, sp, #4`          → Add sp+4 to the stack pointer (set up the new frame pointer)
            // "\tsub\tsp, sp, #8\n"           // `sub   sp, sp, #8`          → Lower the stack pointer by 8
            // "\tstr\tr0, [fp, #-8]\n",       // `str   r0, [fp, #-8]`       → Store the first argument (in r0) into the local variable slot (ARM passes first arg in r0; this stores it into the local frame at offset -8 from fp).
            name, name);
}

/// @brief Print out the assembly postamble
void cgfuncpostamble(int id)
{
    cglabel(Gsym[id].endlabel); // Mark <endlabel>
    fputs(
        "\tldp\tx29, x30, [sp], 16\n" // `ldp     x29, x30, [sp], 16` restore FP & LR; adjust SP back
        "\tret\n"                     // `ret` return to caller
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
        /// `movz <reg>, #<value>`

        fprintf(Outfile, "\tmovz\t%s, #%d\n", reglist[r], value);
    }
    else
    {
        load_int_label(value);
        fprintf(Outfile,
                "\tldr\t%s, [x3]\n",
                reglist[r]);
    }

    return r;
}

/// @brief Generate code to load address of global
/// variable to a register
static void load_global_var_addr(const char *var_name, const char *r_name)
{
    fprintf(Outfile,
            "\tadrp\t%s, __global_%s@PAGE\n"
            "\tadd\t%s, %s, __global_%s@PAGEOFF\n",
            r_name, var_name, r_name, r_name, var_name);
}

/// @brief Generate code to load global variable addr to `x3`
static void load_var_symbol(int id)
{
    load_global_var_addr(Gsym[id].name, "x3");
}

/// @brief Load a value from a variable into a register.
/// Return the number of the register
int cgloadglob(int id)
{
    // Get a new register
    int r = alloc_register();

    // Get the offset to the variable
    load_var_symbol(id);

    // Print out the code to initialise it
    switch (Gsym[id].type)
    {
    case P_CHAR:
        fprintf(Outfile, "\tldrb\t%s, [x3]\n", dreglist[r]);
        break;
    case P_INT:
        fprintf(Outfile, "\tldr\t%s, [x3]\n", dreglist[r]);
        break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
        fprintf(Outfile, "\tldr\t%s, [x3]\n", reglist[r]);
        break;
    default:
        fatald("Bad type in cgloadglob:", Gsym[id].type);
    }
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
    /// `sdiv r1, r1, r2`
    ///

    fprintf(Outfile, "\tsdiv\t%s, %s, %s\n", reglist[r1], reglist[r1], reglist[r2]);
    free_register(r2);
    return (r1);
}

/// @brief printint() with the given register
/// @deprecated
__deprecated void cgprintint(int r)
{
    fprintf(Outfile, "\tmov\tx0, %s\n", reglist[r]);
    fprintf(Outfile, "\tbl\tprintint\n");
    free_register(r);
}

/// @brief Call a function with one argument from the given register
/// Return the register with the result
int cgcall(int r, int id)
{
    fprintf(Outfile, "\tmov\tx0, %s\n", reglist[r]);
    fprintf(Outfile, "\tbl\t%s\n", Gsym[id].name);
    fprintf(Outfile, "\tmov\t%s, x0\n", reglist[r]);
    return r;
}

/// @brief Shift a register left by a constant
int cgshlconst(int r, int val)
{
    fprintf(Outfile, "\tlsl\t%s, %s, #%d\n", reglist[r], reglist[r], val);
    return (r);
}

/// @brief Store a register's value into a variable
int cgstorglob(int r, int id)
{
    // Get the offset to the variable
    load_var_symbol(id);

    switch (Gsym[id].type)
    {
    case P_CHAR:
        fprintf(Outfile, "\tstrb\t%s, [x3]\n", dreglist[r]); // 8-bit store
        break;
    case P_INT:
        fprintf(Outfile, "\tstr\t%s, [x3]\n", dreglist[r]); // 32-bit store (use wX)
        break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
        fprintf(Outfile, "\tstr\t%s, [x3]\n", reglist[r]); // 64-bit store (use xX)
        break;
    default:
        fatald("Bad type in cgstorglob:", Gsym[id].type);
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
    [P_LONG] = 8,
    [P_VOIDPTR] = 8,
    [P_CHARPTR] = 8,
    [P_INTPTR] = 8,
    [P_LONGPTR] = 8};

/// @brief Given a P_XXX type value, return the
/// size of a primitive type in bytes.
int cgprimsize(int type)
{
    // Check the type is valid
    if (type < P_NONE || type > P_LONGPTR)
        fatal("Bad type in cgprimsize()");
    return psize[type];
}

/// @note Generate a global symbol
void cgglobsym(int id)
{
    if (Gsym[id].stype == S_FUNCTION)
        return;

    char *name = Gsym[id].name;
    int type = Gsym[id].type;

    if (Gsym[id].stype == S_ARRAY)
        if (ptrtype(type))
            type = value_at(type);

    int typesize = genprimsize(type);
    int align = log2(typesize);

    fprintf(Outfile,
            "\t.bss\n"
            "\t.align %d\n"
            "\t.globl __global_%s\n"
            "__global_%s:\n"
            "\t.space %d\n"
            "\n",
            align, name, name, typesize * Gsym[id].size);
}

// List of comparison instruction
static char *cmplist[] = {
    "eq", // A_EQ
    "ne", // A_NE
    "lt", // A_LT
    "gt", // A_GT
    "le", // A_LE
    "ge", // A_GE
};

/// @brief Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
    fprintf(Outfile, "\tcset\t%s, %s\n", reglist[r2], cmplist[ASTop - A_EQ]); // set 1 if true else 0
    // fprintf(Outfile, "\tuxtb\t%s, %s\n", reglist[r2], reglist[r2]);           // zero-extend 8-bit
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

    fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]); // compare r1 - r2
    fprintf(Outfile, "\t%s\tL%d\n", brlist[ASTop - A_EQ], label);  // branch if false
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

    fprintf(Outfile, "\tmov\tx0, %s\n", reglist[reg]);
    cgjump(Gsym[id].endlabel);
}

/// @brief Generate code to load the address of a global
/// identifier into a variable. Return a new register
int cgaddress(int id)
{
    // Get a new register
    int r = alloc_register();
    // Get the offset to the variable
    load_global_var_addr(Gsym[id].name, reglist[r]);

    return r;
}

/// @brief Dereference a pointer to get the value it
/// pointing at into the same register
int cgderef(int r, int type)
{
    /// @note
    /// `ldr w0, [x0]` // Load the 32-bit value *p into w0
    ///
    switch (type)
    {
    case P_CHARPTR:
        fprintf(Outfile, "\tldrb\t%s, [%s]\n", dreglist[r], reglist[r]);
        break;
    case P_INTPTR:
        fprintf(Outfile, "\tldr\t%s, [%s]\n", dreglist[r], reglist[r]);
        break;
    case P_LONGPTR:
        fprintf(Outfile, "\tldr\t%s, [%s]\n", reglist[r], reglist[r]);
        break;
    }
    return r;
}

/// @brief Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type)
{
    switch (type)
    {
    case P_CHAR:
        fprintf(Outfile, "\tstrb\t%s, [%s]\n", dreglist[r1], reglist[r2]);
        break;
    case P_INT:
        fprintf(Outfile, "\tstr\t%s, [%s]\n", dreglist[r1], reglist[r2]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tstr\t%s, [%s]\n", reglist[r1], reglist[r2]);
        break;
    default:
        fatald("Can't cgstoderef on type:", type);
    }
    return r1;
}

// #endregion