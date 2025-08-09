# Regular language

A **regular grammar** (or [**regular language**](https://en.wikipedia.org/wiki/Regular_language)) is one that can be recognized by a finite automation - i.e., no memory beyond the current state is needed. This is what traditional regex-based lexers (like [Flex](<https://en.wikipedia.org/wiki/Flex_(lexical_analyser_generator)>)) can handle: fixed patterns, independent of context.  
Most programming languages (C, Java, Go) have regular lexical grammars - their tokenization can be done with regex-like rules plus a few modes. But Python and Haskell are **not regular**, they require **stateful lexing** because _layout is part of the lexical grammar_, not just the parser.

### What _not regular_ means

If a language's **lexical grammar** is not regular, it means:

- Your **cannot** write its lexical rules entirely as regular expressions.
- Token recognition requires **extra state of memory** beyond a finite automation.
- A purely regular-expression-based lexer cannot do it without hacks. You need extra processing logic.

In other words:

> The process of breaking the source code into tokens depends on context-sensitive information, not just fixed character patterns.

### Why Python's lexical grammar is not regular

Python's lexing depends on **indentation levels** (`INDENT` and `DEDENT` tokens). To know when to emit an `INDENT` and `DEDENT`, you must **compare the current line's indentation to previous lines**. This require a **stack** of indentation levels - which is **not finite-state** (in theory, it's unbounded memory).

Example:

```python
def foo():
    if x:
        print("hi")
    print("done")
```

A finite automation cannot handle 'when indentation decreases, emit the right number of `DEDENT`s' - it needs memory to store the indentation stack.

### Why Haskell's lexical grammar is not regular

Haskell also has **layout rules** (the "offside rule").

- You can omit explicit `{` `}` and `;` - indentation determines block structure.
- To know where implicit braces/semicolons go, you must **remember column position** of earlier tokens. Again, that's a **stack-like memory requirement**, not just finite states.
  Example:

```haskell
f x = case x of
    1 -> "one"
    2 -> "two"
```

The lexer has to insert `{` and `;` tokens automatically based on indentation, which means it must compare current indentation with stored values from earlier lines.
