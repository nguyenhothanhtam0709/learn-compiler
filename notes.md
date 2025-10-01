##

If you are building a compiler from scratch (without ready-made backends like LLVM), it is worth focusing on two fronts: intermediate representation and optimization techniques.

Recommended readings

Engineering a Compiler (Cooper & Torczon) – covers data flow optimizations, loop transformations, interprocedural analysis, and more.

Materials about SSA (Static Single Assignment) – intermediate form widely used for optimizations such as constant propagation, dead code elimination, global value numbering, loop-invariant code motion and strength reduction.

Advanced Compiler Design and Implementation (Muchnick) – classic reference for high- and low-level optimizations.

Why SSA is important SSA simplifies data flow analysis and facilitates various optimizations as each variable is assigned exactly once, making dependencies explicit.

Some optimizations to study

Local scope: constant folding, copy propagation, peephole optimizations.

Global scope: dead code elimination, common subexpression elimination, global value numbering.

Loops: loop unrolling, loop-invariant code motion, strength reduction.

Advanced analysis: alias analysis, escape analysis, register allocation via interference graph.

For concrete examples, it is worth consulting papers on SSA and examining minimal implementations of optimizations in Tiger Compiler, Cranelift, or in the backend of languages such as LuaJIT and WebAssembly.

##

[Intermediate Representations](https://compilerprogramming.github.io/intermediate-representations.html).  
[Optimizing Compiler for Register VM](https://github.com/CompilerProgramming/ez-lang/blob/main/optvm/README.md).  
[Learning Resources](https://compilerprogramming.github.io/learning-resources.html).

[Sea of node simple](https://github.com/SeaOfNodes/Simple).
[Sea of node ir](https://github.com/CompilerProgramming/ez-lang/tree/main/seaofnodess).
