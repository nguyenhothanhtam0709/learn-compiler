#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

// #region Basic register allocator

#define REG_NUM 4

/// @brief List of available registers and their names
static int freereg[REG_NUM];
/// @brief List of registers used by compiler
static char *reglist[REG_NUM] = {"%r8", "%r9", "%r10", "%r11"};

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

    fprintf(stderr, "Out of registers!\n");
    exit(EXIT_FAILURE);
}

/// @brief Return a register to the list of available registers.
/// Check to see if it's not already there.
static void free_register(int reg)
{
    if (freereg[reg] != 0)
    {
        fprintf(stderr, "Error trying to free register %d\n", reg);
        exit(EXIT_FAILURE);
    }

    freereg[reg] = 1;
}

// #endregion

// #region ASM code gen

/// @brief Print out the assembly preamble
void cgpreamble()
{
    freeall_registers();
    fputs(
        "\t.text\n"                 // `.text`                  → begin code section
        ".LC0:\n"                   // `.LC0`                   → label for string literal
        "\t.string\t\"%d\\n\"\n"    // `.string "%d\n"`         → define format string
        "printint:\n"               // `printint:`              → start of function printint
        "\tpushq\t%rbp\n"           // `pushq %rbp`             → save old base pointer
        "\tmovq\t%rsp, %rbp\n"      // `movq %rsp, %rbp`        → establish new stack frame
        "\tsubq\t$16, %rsp\n"       // `subq $16, %rsp`         → allocate 16 bytes of stack space
        "\tmovl\t%edi, -4(%rbp)\n"  // `movl %edi, -4(%rbp)`    → save int argument to local var
        "\tmovl\t-4(%rbp), %eax\n"  // `movl -4(%rbp), %eax`    → load local var into eax
        "\tmovl\t%eax, %esi\n"      // `movl %eax, %esi`        → move value into 2nd arg for printf
        "\tleaq	.LC0(%rip), %rdi\n" // `leaq .LC0(%rip), %rdi`  → load address of format string
        "\tmovl	$0, %eax\n"         // `movl $0, %eax`          → set varargs register count to 0
        "\tcall	printf@PLT\n"       // `call printf@PLT`        → call printf
        "\tnop\n"                   // `nop`                    → no operation
        "\tleave\n"                 // `leave`                  → restore base pointer & stack
        "\tret\n"                   // `ret`                    → return to caller
        "\n"
        "\t.globl\tmain\n"           // `.globl main`           → export main symbol
        "\t.type\tmain, @function\n" // `.type main, @function` → declare main as function
        "main:\n"                    // `main:`                 → start of function main
        "\tpushq\t%rbp\n"            // `pushq %rbp`            → save old base pointer
        "\tmovq	%rsp, %rbp\n",       // `movq %rsp, %rbp`       → establish new stack frame
        Outfile);
}

/// @brief Print out the assembly postamble
void cgpostamble()
{
    fputs(
        "\tmovl	$0, %eax\n" // `movl $0, %eax`  → set return value of function to 0 (C convention)
        "\tpopq	%rbp\n"     // `popq %rbp`      → restore old base pointer (undo function prologue)
        "\tret\n",          // `ret`            → return to caller
        Outfile);
}

/// @brief Load an integer literal value into a register.
/// Return the number of the register
int cgload(int value)
{
    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
    return r;
}

/// @brief Add two registers together and return
/// the number of the register with the result
int cgadd(int r1, int r2)
{
    /// @brief r2 = r2 + r1
    ///
    /// @note `addq %r1, %r2`

    fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]); // `addq %r1, %r2`
    free_register(r1);
    return r2;
}

/// @brief Subtract the second register from the first and
/// return the number of the register with the result
int cgsub(int r1, int r2)
{
    /// @brief r1 = r1 - r2
    ///
    /// @note `subq %r2, %r1`

    fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]); // `subq %r2, %r1`
    free_register(r2);
    return r1;
}

/// @brief Multiply two registers together and return
/// the number of the register with the result
int cgmul(int r1, int r2)
{
    /// @brief r2 = r2 * r1
    ///
    /// @note `imulq %r1, %r2`

    fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]); // `imulq %r1, %r2`
    free_register(r1);
    return r2;
}

/// @brief Divide the first register by the second and
/// return the number of the register with the result
int cgdiv(int r1, int r2)
{
    /// @brief r1 = r1 / r2
    ///
    /// @note We need to load `%rax` with the dividend from `r1`.
    /// This needs to be extended to eight bytes with `cqo`.
    /// Then, `idivq` will divide `%rax` with the divisor in `r2`,
    /// leaving the quotient in `%rax`, so we need to copy it out to either `r1` or `r2`.
    /// Then we can free the other register.
    ///
    /// `movq %r1, %rax`    - Division on x86-64 must use `%rax` as the dividend register.
    /// `cgo`               - `cqo` sign-extends `%rax` into `%rdx:%rax`.
    ///                         After this, `%rdx:%rax` forms a 128-bit signed dividend.
    ///                         Needed because `idivq` divides a 128-bit number by a 64-bit divisor.
    /// `idivq %r2`         - Divide the 128-bit value in `%rdx:%rax` by the 64-bit signed value in `reglist[r2]`.
    ///                         Result:
    ///                           + Quotient → stored in `%rax`.
    ///                           + Remainder → stored in `%rdx`.
    /// `movq %rax, %r1`    - Copy the quotient from `%rax` back into the original destination register `reglist[r1]`.
    ///

    fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]); // `movq %r1, %rax`
    fprintf(Outfile, "\tcqo\n");                         // `cgo`
    fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);      // `idivq %r2`
    fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]); // `movq %rax, %r1`
    free_register(r2);
    return (r1);
}
/// @brief printint() with the given register
void cgprintint(int r)
{
    /// @note There isn't an x86-64 instruction to print a register out as a decimal number.
    /// To solve this problem, the assembly preamble contains a function called `printint()`
    /// that takes a register argument and calls `printf()` to print this out in decimal.
    ///
    /// Linux x86-64 expects the first argument to a function to be in the `%rdi` register,
    /// so we move our register into `%rdi` before we call `printint`.
    ///

    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    free_register(r);
}

// #endregion