///
/// x64-64 code generator
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
        "\n",
        Outfile);
}

// Print out a function preamble
void cgfuncpreamble(char *name)
{
    fprintf(Outfile,
            "\t.text\n"
            "\t.globl\t%s\n"           // `.globl <name>`           → Declare <name> as a global symbol, visible to the linker
            "\t.type\t%s, @function\n" // `.type <name>, @function` → Mark <name> as a function symbol (for debuggers/linkers)
            "%s:\n"                    // `<name>:`                 → Define the label <name> (entry point of the function)
            "\tpushq\t%%rbp\n"         // `pushq %rbp`              → Save caller's base pointer on the stack
            "\tmovq\t%%rsp, %%rbp\n",  // `movq %rsp, %rbp`         → Set up a new stack frame: rbp = current stack pointer
            name, name, name);
}

/// @brief Print out the assembly postamble
void cgfuncpostamble()
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

// List of comparison instruction
static char *cmplist[] = {
    "sete",  // A_EQ
    "setne", // A_NE
    "setl",  // A_LT
    "setg",  // A_GT
    "setle", // A_LE
    "setge", // A_GE
};

/// @brief Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    /// @note
    /// `cmpq %r2, %r1`                 → This does `%r1 - %r2` and sets flags (ZF, SF, OF, CF) accordingly.
    /// `(how) (breglist[r2])`          → A conditional set instruction (`setcc`) that writes 1 (true) or 0 (false)
    ///                                     to `breglist[r2]` (8-bit version of r2 register), based on the flags.
    ///                                     Some `how` instructions:
    ///                                         + `sete`    - set if equal (ZF = 1)
    ///                                         + `setne`   - set if not equal (ZF = 0)
    ///                                         + `setl`    - set if signed less (SF != OF)
    ///                                         + `setle`   - set if signed less or equal (ZF = 1 or SF != OF)
    ///                                         + `setg`    - set if signed greater (ZF = 0 and SF == OF)
    ///                                         + `setge`   - sset if signed greater or equal (SF == OF)
    ///                                     Because x86 conditional set instructions (sete, setne, setl, etc.) only work on 8-bit registers,
    ///                                     So, we must use 8-bit version of normal register (`breglist[r2]`).
    /// `movzbq %(breglist[r2]), %r2`   → Zero-extends the 8-bit boolean result in `%(breglist[r2])` (0 or 1)
    ///                                   into the full 64-bit register `r2`, clearing the upper 56 bits.

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
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
    /// @note `jmp L<label>` - Emit an unconditional jump to a specified label.

    fprintf(Outfile, "\tjmp\tL%d\n", l);
}

/// @brief List of inverted jump instructions
static char *invcmplist[] = {
    "jne", // A_EQ
    "je",  // A_NE
    "jge", // A_LT
    "jle", // A_GT
    "jg",  // A_LE
    "jl",  // A_GE
};

/// @brief Compare two registers and jump if false
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    /// @note
    /// `cmpq %r2, %r1`                                → Performs `%r1 - %r2` and sets condition flags (ZF, SF, OF, CF).
    /// `(invcmplist[ASTop - A_EQ]) (breglist[r2])`    → Emit conditional jump instruction to `L<label>` based on the inverted condition
    ///                                                  Some `invcmplist[ASTop - A_EQ]` instructions:
    ///                                                     + `jne`  - jump if not equal (ZF = 0)
    ///                                                     + `je`   - jump if equal (ZF = 1)
    ///                                                     + `jge`  - jump if greater or equal (SF == OF)
    ///                                                     + `jle`  - jump if less or equal (ZF = 1 or SF != OF)
    ///                                                     + `jg`   - jump if greater (ZF = 0 and SF == OF)
    ///                                                     + `jl`   - jump if less (SF != OF)

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
    freeall_registers();

    return NOREG;
}

// #endregion