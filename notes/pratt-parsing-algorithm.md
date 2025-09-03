# Pratt parsing algorithm

A **Pratt parser** is a _top-down operator-precedence parser_. Instead of encoding precedence rules in a grammar (like in yacc/BNF), it gives each token two parsing functions:

- **prefix function** → how to parse the token if it appears at the beginning of an expression (e.g., numbers, unary -, parentheses).
- **infix function** → how to parse the token if it appears in the middle (e.g., `+`, `*`, `==`).

Each operator also has a binding power (precedence level) that determines whether the parser should “stop” parsing at a certain operator.

### Example

For input

```js
1 + 2 * 3;
```

- Start: parse `1` using prefix rule for numbers → AST node 1.
- See `+`. Precedence = `TERM`.
- Parse RHS with precedence higher than `TERM`.
  - RHS begins with `2`. Prefix rule → node `2`.
  - Next token is `*`. Precedence = `FACTOR` (higher than `TERM`), so parse it.
  - Parse RHS of `*`: number `3`.
  - Build AST (`2 * 3`).
- Combine with `+`: (`1 + (2 * 3)`).
