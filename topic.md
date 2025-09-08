# Topic

- Graph coloring, linear scan, and priority coloring approaches to register allocation.
- Sea of nodes.
- Abstract interpretation
- Types, points to sets, abstract heaps, and the ways that these things are the same
- Instruction selection

### Types of IRs

- Static single-assignment (SSA) form
- Control flow graph
- Continuation-passing style
- Three-address code

### Some optimizations

- Constant propagation
- Common subexpression elimination
- Loop invariant code motion
- Global value numbering
- Strength reduction
- Scalar replacement of aggregates
- Dead code elimination
- Loop unrolling

### Some techniques to implement bytecode dispatching for bytecode interpreter
- Switch dispatch: basic bytecode dispatching technique used in clox
- Computed Goto / Direct Threading (Direct threaded code) / Jump table
- Indirect Threading
- Direct Call Threading
- Superinstructions
- Inline Threading / Replication
- Token Threading (very old-school, Forth style)