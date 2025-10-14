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

void fprint_escaped(FILE *f, const char *s)
{
    for (; *s; s++)
    {
        switch (*s)
        {
        case '\n':
            fprintf(f, "\\n");
            break;
        case '\t':
            fprintf(f, "\\t");
            break;
        case '\\':
            fprintf(f, "\\\\");
            break;
        case '\"':
            fprintf(f, "\\\"");
            break;
        default:
            if ((unsigned char)*s < 32 || (unsigned char)*s > 126)
                fprintf(f, "\\x%02x", (unsigned char)*s);
            else
                fputc(*s, f);
        }
    }
}

/// @brief Maximum amount of arguments can be
/// passed in register for function call.
/// The remaining must be pushed into stack.
#define MAX_ARGS_IN_REG 8

/// @brief Position of next local variable relative to stack base pointer.
/// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
static int stackOffset;

/// @brief Create the position of a new local variable.
int newlocaloffset(int type)
{
    return -(localOffset += cgprimsize(type));
}

// #region Basic register allocator

#define NUMFREEREGS 4
/// @brief Position of first parameter register
#define FIRSTPARAMREG 11

/// @brief List of available registers and their names
static int freereg[NUMFREEREGS];
/// @brief List of registers used by compiler
static char *reglist[] = {"x9", "x10", "x11", "x12",
                          /// @note Registers for function call arguments
                          "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0"};
/// @brief List of registers that are lower 32-bit version of each register of `reglist`
static char *dreglist[] = {"w9", "w10", "w11", "w12",
                           /// @note Registers for function call arguments
                           "w7", "w6", "w5", "w4", "w3", "w2", "w1", "w0"};

/// @brief Set all registers as available
void freeall_registers(void)
{
    for (int i = 0; i < NUMFREEREGS; i++)
        freereg[i] = 1;
}

/// @brief Allocate a free register. Return the number of
/// the register. Die if no available registers.
static int alloc_register(void)
{
    for (int i = 0; i < NUMFREEREGS; i++)
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

#define MAXSTRS 1024
static struct
{
    char *s;
    int l;
} Strlist[MAXSTRS];
static int Strslot = 0;

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
        "\t.extern _write\n"
        "\n"
        "\t.cstring\n"
        "msgfmt:\n"
        "\t.asciz \"%d\\n\"\n"
        "\n"
        // #region Define function `printint`
        "\t.text\n"                       // `.text`              → begin code section
        "\t.global _printint\n"           // `.global _printint`
        "_printint:\n"                    // `_printint:`
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
        "\n"
        // #region Define function `printchar`
        "\t.text\n"
        "\t.global _printchar\n"
        "_printchar:\n"
        "\tstp\tx29, x30, [sp, -16]!\n"
        "\tmov\tx29, sp\n"
        "\tand\tw0, w0, #0x7f\n" // x & 0x7f → mask character
        "\tsub\tsp, sp, #16\n"   // allocate 16 bytes on stack
        "\tstrb\tw0, [sp]\n"     // store character byte
        "\tmov\tx0, #1\n"        // fd = 1 (stdout)
        "\tmov\tx1, sp\n"        // buffer = &char
        "\tmov\tx2, #1\n"        // count = 1
        /// @note `putc` needs pointer to `STDOUT` Apple’s libc hides it as an internal weak reference,
        /// meaning it doesn’t appear in the dynamic symbol table. So we use system call `write`, which
        /// accepts file descriptor directly.
        "\tbl\t_write\n"       // write(fd, &char, 1)
        "\tadd\tsp, sp, #16\n" // deallocate stack
        "\tldp\tx29, x30, [sp], 16\n"
        "\tret\n"
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
    //     if (Symtable[i].stype == S_VARIABLE)
    //     {
    //         char *name = Symtable[i].name;
    //         int typesize = genprimsize(Symtable[i].type);
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

    for (int i = 0; i < Globs; i++)
    {
        if (Symtable[i].stype != S_FUNCTION)
            continue;

        fprintf(Outfile,
                "\t.extern _%s\n",
                Symtable[i].name);
    }
    fputc('\n', Outfile);

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
    // fprintf(Outfile, "\t.section __TEXT,__cstring\n"); -> `.cstring` is read-only data
    fprintf(Outfile, "\t.data\n"); // `.data` is initialized writable data
    for (int i = 0; i < Strslot; i++)
    {
        int lidx = Strlist[i].l;
        fprintf(Outfile,
                "__strconst_%d:\n"
                "\t.asciz \"",
                lidx);
        fprint_escaped(Outfile, Strlist[i].s);
        fputs("\"\n", Outfile);
    }
    fputs("\n", Outfile);
    // #endregion
}

// Print out a function preamble
void cgfuncpreamble(int id)
{
    char *name = Symtable[id].name;

    int i;
    /// @note Any pushed params start at this stack offset
    int paramOffset = 16;
    /// @brief Index to the first param register in above reg lists
    int paramReg = FIRSTPARAMREG;
    localOffset = 0;

    fprintf(Outfile,
            "\t.text\n"
            "\t.global\t_%s\n"              // `.global <name>`             → Declare <name> as a global symbol, visible to the linker
            "_%s:\n"                        // `<name>:`                   → Define the label <name> (entry point of the function)
            "\tstp\tx29, x30, [sp, -16]!\n" // `stp     x29, x30, [sp, -16]!` push FP (x29) and LR (x30); update SP
            "\tmov\tx29, sp\n",             // `mov     x29, sp` set new frame pointer
            // "\tadd\tfp, sp, #4\n"           // `add   fp, sp, #4`          → Add sp+4 to the stack pointer (set up the new frame pointer)
            // "\tsub\tsp, sp, #8\n"           // `sub   sp, sp, #8`          → Lower the stack pointer by 8
            // "\tstr\tr0, [fp, #-8]\n",       // `str   r0, [fp, #-8]`       → Store the first argument (in r0) into the local variable slot (ARM passes first arg in r0; this stores it into the local frame at offset -8 from fp).
            name, name);

    // Calculate offset of parameter before loading them from register into stack
    for (i = NSYMBOLS - 1; (i > Locls) && (i >= NSYMBOLS - MAX_ARGS_IN_REG); i--)
    {
        if (Symtable[i].class != C_PARAM)
            break;

        Symtable[i].posn = newlocaloffset(Symtable[i].type);
    }

    // For the remainder, if they are a parameter then they are
    // already on the stack. If only a local, make a stack position.
    for (; i > Locls; i--)
    {
        if (Symtable[i].class == C_PARAM)
        {
            /// @note In AArch64 ABI, only the first 8 parameters of function are
            /// allocated on the register, the rest of parameters will be allocated
            /// on the stack of caller. Below code calculates stack offset of stack-allocated
            /// parameter relative to `%rbp`
            Symtable[i].posn = paramOffset;
            paramOffset += 8;
        }
        else
            Symtable[i].posn = newlocaloffset(Symtable[i].type);
    }

    /// @note Keep stack 16-bytes align as requirement of AArch64 ABI
    stackOffset = (localOffset + 15) & ~15;
    fprintf(Outfile,
            "\tsub\tsp, sp, #%d\n",
            stackOffset);

    // Copy any in-register parameters to the stack
    // Stop after no more than six parameter registers
    for (int idx = NSYMBOLS - 1; (idx > Locls) && (idx >= NSYMBOLS - MAX_ARGS_IN_REG); idx--)
    {
        if (Symtable[idx].class != C_PARAM)
            break;

        cgstorlocal(paramReg--, idx);
    }
}

/// @brief Print out the assembly postamble
void cgfuncpostamble(int id)
{
    cglabel(Symtable[id].endlabel); // Mark <endlabel>
    fprintf(Outfile,
            "\tadd\tsp, sp, #%d\n"
            "\tldp\tx29, x30, [sp], 16\n" // `ldp     x29, x30, [sp], 16` restore FP & LR; adjust SP back
            "\tret\n"                     // `ret` return to caller
            "\n",
            stackOffset);
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
    load_global_var_addr(Symtable[id].name, "x3");
}

/// @brief Load a value from a variable into a register.
/// Return the number of the register. If the
/// operation is pre- or post-increment/decrement,
/// also perform this action.
int cgloadglob(int id, int op)
{
    // Get a new register
    int r = alloc_register();

    /// @note AArch64 doesn’t have a single instruction that increments memory directly (no incb [addr]).
    /// Instead, we must:
    /// - Load the byte
    /// - Increment or decrement it
    /// - Store it back.

    // Get the offset to the variable
    load_var_symbol(id);
    if (cgprimsize(Symtable[id].type) == 8)
    {
        /// @note Register for computation
        const char *computeR = "x4";
        if (op == A_PREINC || op == A_PREDEC)
            computeR = reglist[r];

        /// @note Load value into allocated register `%r` for result
        fprintf(Outfile, "\tldr\t%s, [x3]\n", reglist[r]);

        if (A_POSTINC == op || A_POSTDEC == op)
            /// @note Load value into temporary register for compute
            fprintf(Outfile, "\tldr\t%s, [x3]\n", computeR);

        if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
            fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
        else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
            fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

        if (A_PREINC == op ||
            A_POSTINC == op ||
            A_POSTDEC == op ||
            A_POSTINC == op)
            /// @note Store value back to global variable
            fprintf(Outfile, "\tstr\t%s, [x3]\n", computeR);
    }
    else
    {
        // Print out the code to initialise it
        switch (Symtable[id].type)
        {
        case P_CHAR:
        {
            /// @note Register for computation
            const char *computeR = "w4";
            if (op == A_PREINC || op == A_PREDEC)
                computeR = dreglist[r];

            /// @note Load value into allocated register `%r` for result
            fprintf(Outfile, "\tldrb\t%s, [x3]\n", dreglist[r]);

            if (A_POSTINC == op || A_POSTDEC == op)
                /// @note Load value into temporary register for compute
                fprintf(Outfile, "\tldrb\t%s, [x3]\n", computeR);

            if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
                fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
            else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
                fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

            if (A_PREINC == op ||
                A_POSTINC == op ||
                A_POSTDEC == op ||
                A_POSTINC == op)
                /// @note Store value back to global variable
                fprintf(Outfile, "\tstrb\t%s, [x3]\n", computeR);

            break;
        }
        case P_INT:
        {
            /// @note Register for computation
            const char *computeR = "w4";
            if (op == A_PREINC || op == A_PREDEC)
                computeR = dreglist[r];

            /// @note Load value into allocated register `%r` for result
            fprintf(Outfile, "\tldr\t%s, [x3]\n", dreglist[r]);

            if (A_POSTINC == op || A_POSTDEC == op)
                /// @note Load value into temporary register for compute
                fprintf(Outfile, "\tldr\t%s, [x3]\n", computeR);

            if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
                fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
            else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
                fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

            if (A_PREINC == op ||
                A_POSTINC == op ||
                A_POSTDEC == op ||
                A_POSTINC == op)
                /// @note Store value back to global variable
                fprintf(Outfile, "\tstr\t%s, [x3]\n", computeR);

            break;
        }
        default:
            fatald("Bad type in cgloadglob:", Symtable[id].type);
        }
    }
    return r;
}

/// @brief Load a value from a local variable into a register.
/// Return the number of the register. If the
/// operation is pre- or post-increment/decrement,
/// also perform this action.
int cgloadlocal(int id, int op)
{
    // Get a new register
    int r = alloc_register();
    /// @note Stack offset
    int posn = Symtable[id].posn;

    if (cgprimsize(Symtable[id].type) == 8)
    {
        /// @note Register for computation
        const char *computeR = "x4";
        if (op == A_PREINC || op == A_PREDEC)
            computeR = reglist[r];

        /// @note Load value into allocated register `%r` for result
        fprintf(Outfile, "\tldr\t%s, [x29, #%d]\n", reglist[r], posn);

        if (A_POSTINC == op || A_POSTDEC == op)
            /// @note Load value into temporary register for compute
            fprintf(Outfile, "\tldr\t%s, [x3]\n", computeR);

        if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
            fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
        else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
            fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

        if (A_PREINC == op ||
            A_POSTINC == op ||
            A_POSTDEC == op ||
            A_POSTINC == op)
            /// @note Store value back to global variable
            fprintf(Outfile, "\tstr\t%s, [x29, #%d]\n", computeR, posn);
    }
    switch (Symtable[id].type)
    {
    case P_CHAR:
    {
        /// @note Register for computation
        const char *computeR = "w4";
        if (op == A_PREINC || op == A_PREDEC)
            computeR = dreglist[r];

        /// @note Load value into allocated register `%r` for result
        fprintf(Outfile, "\tldrb\t%s, [x29, #%d]\n", dreglist[r], posn);

        if (A_POSTINC == op || A_POSTDEC == op)
            /// @note Load value into temporary register for compute
            fprintf(Outfile, "\tldrb\t%s, [x3]\n", computeR);

        if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
            fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
        else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
            fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

        if (A_PREINC == op ||
            A_POSTINC == op ||
            A_POSTDEC == op ||
            A_POSTINC == op)
            /// @note Store value back to global variable
            fprintf(Outfile, "\tstrb\t%s, [x29, #%d]\n", computeR, posn);

        break;
    }
    case P_INT:
    {
        /// @note Register for computation
        const char *computeR = "w4";
        if (op == A_PREINC || op == A_PREDEC)
            computeR = dreglist[r];

        /// @note Load value into allocated register `%r` for result
        fprintf(Outfile, "\tldr\t%s, [x29, #%d]\n", dreglist[r], posn);

        if (A_POSTINC == op || A_POSTDEC == op)
            /// @note Load value into temporary register for compute
            fprintf(Outfile, "\tldr\t%s, [x3]\n", computeR);

        if (A_PREINC == op || A_POSTINC == op) // A_PREINC, A_POSTINC
            fprintf(Outfile, "\tadd\t%s, %s, #1\n", computeR, computeR);
        else if (A_PREDEC == op || A_POSTDEC == op) // A_PREDEC, A_POSTDEC
            fprintf(Outfile, "\tsub\t%s, %s, #1\n", computeR, computeR);

        if (A_PREINC == op ||
            A_POSTINC == op ||
            A_POSTDEC == op ||
            A_POSTINC == op)
            /// @note Store value back to global variable
            fprintf(Outfile, "\tstr\t%s, [x29, #%d]\n", computeR, posn);

        break;
    }
    default:
        fatald("Bad type in cgloadglob:", Symtable[id].type);
    }
    return r;
}

/// @brief Given the label number of a global string,
/// load its address into a new register
int cgloadglobstr(int l)
{
    // Get a new register
    int r = alloc_register();
    const char *r_name = reglist[r];
    fprintf(Outfile,
            "\tadrp\t%s, __strconst_%d@PAGE\n"
            "\tadd\t%s, %s, __strconst_%d@PAGEOFF\n",
            r_name, l, r_name, r_name, l);
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

int cgand(int r1, int r2)
{
    /// @note `AND` operator and set flags
    fprintf(Outfile, "\tands\t%s, %s, %s\n", reglist[r2], reglist[r1], reglist[r2]);
    free_register(r1);
    return r2;
}

int cgor(int r1, int r2)
{
    /// @note `OR` operator
    fprintf(Outfile,
            "\torr\t%s, %s, %s\n"
            "\tcmp\t%s, #0\n",
            reglist[r2], reglist[r1], reglist[r2], reglist[r2]);

    free_register(r1);
    return r2;
}

int cgxor(int r1, int r2)
{
    /// @note `XOR` operator and set flags
    fprintf(Outfile,
            "\teor\t%s, %s, %s\n"
            "\tcmp\t%s, #0\n",
            reglist[r2], reglist[r1], reglist[r2], reglist[r2]);
    free_register(r1);
    return r2;
}

int cgshl(int r1, int r2)
{
    /// @note `lsl r1, r1, r2` -> `r1 = r1 << (r2 & 0x3F)`
    fprintf(Outfile, "\tlsl\t%s, %s, %s\n",
            reglist[r1], reglist[r1], reglist[r2]);
    free_register(r2);
    return r1;
}

int cgshr(int r1, int r2)
{
    /// @note `lsr r1, r1, r2` -> `r1 = r1 >> (r2 & 0x3F)`
    fprintf(Outfile, "\tlsr\t%s, %s, %s\n",
            reglist[r1], reglist[r1], reglist[r2]);
    free_register(r2);
    return r1;
}

/// @brief Negate a register's value
int cgnegate(int r)
{
    fprintf(Outfile, "\tneg\t%s, %s\n", reglist[r], reglist[r]);
    return r;
}

/// @brief Invert a register's value
int cginvert(int r)
{
    fprintf(Outfile, "\tmvn\t%s, %s\n", reglist[r], reglist[r]);
    return r;
}

/// @brief Logically negate a register's value
int cglognot(int r)
{
    fprintf(Outfile, "\tcmp\t%s, #0\n", reglist[r]);  // set NZCV flags based on x0
    fprintf(Outfile, "\tcset\t%s, eq\n", reglist[r]); // x0 = 1 if equal (ZF=1), else 0
    return r;
}

/// @brief Convert an integer value to a boolean value. Jump if
/// it's an IF or WHILE operation
int cgboolean(int r, int op, int label)
{
    fprintf(Outfile, "\tcmp\t%s, #0\n", reglist[r]);
    if (op == A_IF || op == A_WHILE)
        fprintf(Outfile, "\tbeq\tL%d\n", label);
    else
        fprintf(Outfile, "\tcset\t%s, ne\n", reglist[r]);
    return r;
}

static void cg8bytesstackalloc(int slot)
{
    const int oldStackOffset = stackOffset;
    stackOffset = (localOffset + slot * 8 + 15) & ~15;
    const int allocateOffset = stackOffset - oldStackOffset;
    if (allocateOffset > 0)
        fprintf(Outfile,
                "\tsub\tsp, sp, #%d\n",
                allocateOffset);
}

/// @brief Stack alloc for additional arguments for function calling
void cgargsstackalloc(int numargs)
{
    if (numargs > MAX_ARGS_IN_REG)
        cg8bytesstackalloc(numargs - MAX_ARGS_IN_REG);
}

/// @brief Call a function with the given symbol id
/// Pop off any arguments pushed on the stack
/// Return the register with the result
int cgcall(int id, int numargs)
{
    // Get a new register
    int outr = alloc_register();

    // Call the function
    fprintf(Outfile, "\tbl\t_%s\n", Symtable[id].name);
    // Remove any arguments pushed on the stack
    if (numargs > MAX_ARGS_IN_REG)
    {
        const int stackOffsetBeforeCall = (localOffset + 15) & ~15;
        const int deallocateOffset = stackOffset - stackOffsetBeforeCall;
        if (deallocateOffset > 0)
            fprintf(Outfile,
                    "\tadd\tsp, sp, #%d\n",
                    stackOffset - stackOffsetBeforeCall);
        stackOffset = stackOffsetBeforeCall;
    }
    // and copy the return value into our register
    fprintf(Outfile, "\tmov\t%s, x0\n", reglist[outr]);
    return outr;
}

/// @brief Given a register with an argument value,
/// copy this argument into the argposn'th
/// parameter in preparation for a future function
/// call. Note that argposn is 1, 2, 3, 4, ..., never zero.
void cgcopyarg(int r, int argposn)
{
    // If this is above the sixth argument, simply push the
    // register on the stack. We rely on being called with
    // successive arguments in the correct order for x86-64
    if (argposn > MAX_ARGS_IN_REG)
    {
        const int offset = (argposn - MAX_ARGS_IN_REG - 1) * 8;
        fprintf(Outfile,
                "\tstr\t%s, [sp, #%d]\n",
                reglist[r], offset);
    }
    else
        // Otherwise, copy the value into one of the six registers
        // used to hold parameter values
        fprintf(Outfile,
                "\tmov\t%s, %s\n",
                reglist[FIRSTPARAMREG - argposn + 1], reglist[r]);
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

    if (cgprimsize(Symtable[id].type) == 8)
        fprintf(Outfile, "\tstr\t%s, [x3]\n", reglist[r]); // 64-bit store (use xX)
    else
    {
        switch (Symtable[id].type)
        {
        case P_CHAR:
            fprintf(Outfile, "\tstrb\t%s, [x3]\n", dreglist[r]); // 8-bit store
            break;
        case P_INT:
            fprintf(Outfile, "\tstr\t%s, [x3]\n", dreglist[r]); // 32-bit store (use wX)
            break;
        default:
            fatald("Bad type in cgstorglob:", Symtable[id].type);
        }
    }

    return r;
}

/// @brief Store a register's value into a local variable
int cgstorlocal(int r, int id)
{
    const int posn = Symtable[id].posn;

    if (cgprimsize(Symtable[id].type) == 8)
        fprintf(Outfile, "\tstr\t%s, [x29, #%d]\n", reglist[r], posn);
    else
    {
        switch (Symtable[id].type)
        {
        case P_CHAR:
            fprintf(Outfile, "\tstrb\t%s, [x29, #%d]\n", dreglist[r], posn); // 8-bit store
            break;
        case P_INT:
            fprintf(Outfile, "\tstr\t%s, [x29, #%d]\n", dreglist[r], posn); // 32-bit store (use wX)
            break;
        default:
            fatald("Bad type in cgstorglob:", Symtable[id].type);
        }
    }

    return r;
}

/// @brief Given a P_XXX type value, return the
/// size of a primitive type in bytes.
int cgprimsize(int type)
{
    if (ptrtype(type))
        return 8;

    switch (type)
    {
    case P_CHAR:
        return 1;
    case P_INT:
        return 4;
    case P_LONG:
        return 8;
    default:
        fatald("Bad type in cgprimsize:", type);
    }
    return 0; // Keep -Wall happy
}

/// @note Generate a global symbol
void cgglobsym(int id)
{
    if (Symtable[id].stype == S_FUNCTION)
        return;

    char *name = Symtable[id].name;
    int type = Symtable[id].type;

    if (Symtable[id].stype == S_ARRAY)
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
            align, name, name, typesize * Symtable[id].size);
}

/// @brief Generate a global string and its start label
void cgglobstr(int l, char *strvalue)
{
    if (Strslot == MAXSTRS)
        fatal("Out of string slots in cgglobstr()");

    int idx = Strslot++;
    Strlist[idx].s = strdup(strvalue);
    Strlist[idx].l = l;
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
    cgjump(Symtable[id].endlabel);
}

/// @brief Generate code to load the address of an
/// identifier into a variable. Return a new register
int cgaddress(int id)
{
    // Get a new register
    int r = alloc_register();

    if (Symtable[id].class == C_LOCAL)
        fprintf(Outfile,
                "\tadd\t%s, x29, #%d\n",
                reglist[r], Symtable[id].posn);
    else
        // Get the offset to the variable
        load_global_var_addr(Symtable[id].name, reglist[r]);

    return r;
}

/// @brief Dereference a pointer to get the value it
/// pointing at into the same register
int cgderef(int r, int type)
{
    int valuetype = value_at(type);
    int size = cgprimsize(valuetype);
    /// @note
    /// `ldr w0, [x0]` // Load the 32-bit value *p into w0
    ///
    switch (size)
    {
    case 1:
        fprintf(Outfile, "\tldrb\t%s, [%s]\n", dreglist[r], reglist[r]);
        break;
    case 2:
    case 4:
        fprintf(Outfile, "\tldr\t%s, [%s]\n", dreglist[r], reglist[r]);
        break;
    case 8:
        fprintf(Outfile, "\tldr\t%s, [%s]\n", reglist[r], reglist[r]);
        break;
    }
    return r;
}

/// @brief Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type)
{
    int size = cgprimsize(type);
    switch (size)
    {
    case 1:
        fprintf(Outfile, "\tstrb\t%s, [%s]\n", dreglist[r1], reglist[r2]);
        break;
    case 2:
    case 4:
        fprintf(Outfile, "\tstr\t%s, [%s]\n", dreglist[r1], reglist[r2]);
        break;
    case 8:
        fprintf(Outfile, "\tstr\t%s, [%s]\n", reglist[r1], reglist[r2]);
        break;
    default:
        fatald("Can't cgstoderef on type:", type);
    }
    return r1;
}

// #endregion