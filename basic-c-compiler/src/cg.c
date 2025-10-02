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
/// @brief List of registers that are lower 8-bit version of each register of `reglist`
static char *breglist[REG_NUM] = {"%r8b", "%r9b", "%r10b", "%r11b"};

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
    /// @note Some note about below x86-64 asm:
    ///
    /// The compiler often saves all arguments into the stack frame right at the start,
    /// to make them accessible like local variables. Then, later, when it needs the value again,
    /// it reloads it from the stack rather than reusing the incoming register. That why
    /// we see the instruction
    /// ```asm
    /// movl    %edi, -4(%rbp)     # save printint's 1st argument into stack (or allocate local variable x)
    /// movl    -4(%rbp), %eax     # reload x from stack into eax
    /// movl    %eax, %esi         # copy eax into esi (printf's 2nd argument)
    /// ```
    /// rather than just do `movl %edi, %esi` (move 1st argument of printint directly into esi).
    ///
    /// Although the later would be shorter and faster than above 3-instruction sequence. But
    /// the compilers sometimes choose the “safe and uniform” approach:
    ///     - Always spill function arguments into the stack frame.
    ///     - Later, generate loads from memory when needed.
    ///     - This makes code generation simpler, especially in unoptimized builds (`-O0`).
    /// At higher optimization levels (`-O2`, `-O3`), GCC or Clang will optimize away
    /// the unnecessary memory traffic and just emit `movl %edi, %esi`.
    ///

    freeall_registers();
    fputs(
        "\t.text\n"                 // `.text`                  → begin code section
        ".LC0:\n"                   // `.LC0`                   → label for string literal
        "\t.string\t\"%d\\n\"\n"    // `.string "%d\n"`         → define string constant "%d\n"
        "printint:\n"               // `printint:`              → start of function printint
        "\tpushq\t%rbp\n"           // `pushq %rbp`             → save old base pointer
        "\tmovq\t%rsp, %rbp\n"      // `movq %rsp, %rbp`        → establish new stack frame
        "\tsubq\t$16, %rsp\n"       // `subq $16, %rsp`         → allocate 16 bytes of stack space
        "\tmovl\t%edi, -4(%rbp)\n"  // `movl %edi, -4(%rbp)`    → save 1st arg argument into local var (int x)
        "\tmovl\t-4(%rbp), %eax\n"  // `movl -4(%rbp), %eax`    → load x into eax
        "\tmovl\t%eax, %esi\n"      // `movl %eax, %esi`        → place x into %esi (2nd printf argument)
        "\tleaq	.LC0(%rip), %rdi\n" // `leaq .LC0(%rip), %rdi`  → load address of "%d\n" into rdi (1st arg)
        "\tmovl	$0, %eax\n"         // `movl $0, %eax`          → clear eax (needed for varargs call)
        "\tcall	printf@PLT\n"       // `call printf@PLT`        → call printf("%d\n", x), `@PLT` = Procedure Linkage Table entry
        "\tnop\n"                   // `nop`                    → no operation
        "\tleave\n"                 // `leave`                  → restore base pointer & stack
        "\tret\n"                   // `ret`                    → return to caller
        "\n"
        "\t.globl\tmain\n"           // `.globl main`           → export main symbol: declare main as global
        "\t.type\tmain, @function\n" // `.type main, @function` → mark symbol type: declare main as function
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
int cgloadint(int value)
{
    /// @brief r = value
    ///
    /// @note `movq $value, %r` → Load immediate value `value` to register `r`

    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
    return (r);
}

/// @brief Load a value from a variable into a register.
/// Return the number of the register
int cgloadglob(char *identifier)
{
    /// @brief r = value of identifier
    ///
    /// @note `movq identifier(%rip), %r8` → Take the memory contents at symbol `identifier` (relative to instruction pointer)
    ///                                     and load it to register `r`.
    /// `%rip`-relative addressing is how x86-64 does position-independent code (PIC).
    /// `identifier(%rip)` means "the 64-bit value stored at the address of `identifier`".

    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", identifier, reglist[r]);
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

/// @brief Store a register's value into a variable
int cgstorglob(int r, char *identifier)
{
    /// @brief identifier = r
    ///
    /// @note `movq %r, identifier(%rip)` → Take the content of register `r` and store
    ///                                    it into the memory location of global variable `identifier`.

    fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], identifier);
    return (r);
}

/// @brief Generate a global symbol
void cgglobsym(char *sym)
{
    /// @note `.comm sym, 8, 8` → Tells the assembler/linker to reserve uninitialized storage
    ///                           (like a global variable in C without an initializer).
    ///
    /// `x` → the symbol name (global variable name).
    /// `8` → allocate 8 bytes (size of a 64-bit integer).
    /// `8` → align it on an 8-byte boundary.
    ///
    /// This is exactly what GCC/Clang do for code like `long x;`.
    ///
    /// @details `.comm symbol, length, alignment`

    fprintf(Outfile, "\t.comm\t%s,8,8\n", sym);
}

/// @brief Compare two registers.
static int cgcompare(int r1, int r2, char *how)
{
    /// @note
    /// `cmpq %r2, %r1`               → This does `%r8 - %r9` and sets flags (ZF, SF, OF, CF) accordingly.
    /// `(how) (breglist[r2])`        → Write 0 or 1 to `breglist[r2]` (8-bit version of r2 register) based on `how` instruction
    ///                                 Some `how` instructions:
    ///                                     + `sete`    - set if equal, ZF=1
    ///                                     + `setne`   - set if not equal, ZF=0
    ///                                     + `setl`    - signed less than, SF != OF
    ///                                     + `setle`   - signed less or equal, ZF=1 or SF != OF
    ///                                     + `setg`    - signed greater than, ZF=0 and SF=OF
    ///                                     + `setge`   - signed greater or equal, SF=OF
    ///                                 Because x86 conditional set instructions (sete, setne, setl, etc.) only work on 8-bit registers,
    ///                                 So, we must use 8-bit version of normal register (`breglist[r2]`).
    /// `andq $255, r2`               → This clears the upper 56 bits of `r2`, leaving a clean 0 or 1.
    ///                                 Now `r2` holds the boolean result.

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\t%s\n", how, breglist[r2]);
    fprintf(Outfile, "\tandq\t$255,%s\n", reglist[r2]);
    free_register(r1);
    return r2;
}

int cgequal(int r1, int r2) { return cgcompare(r1, r2, "sete"); }

int cgnotequal(int r1, int r2) { return cgcompare(r1, r2, "setne"); }

int cglessthan(int r1, int r2) { return cgcompare(r1, r2, "setl"); }

int cggreaterthan(int r1, int r2) { return cgcompare(r1, r2, "setg"); }

int cglessequal(int r1, int r2) { return cgcompare(r1, r2, "setle"); }

int cggreaterequal(int r1, int r2) { return cgcompare(r1, r2, "setge"); }

// #endregion