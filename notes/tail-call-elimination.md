# Tail call elimination (Tail call optimization)

We can implement a loop purely by **recursively calling a function** that represents one iteration, passing itself as needed. To optimize, we use **tail call elimination**.  
Tail call elimination (also called *tail call optimization* or TCO) is an optimization that reuses the current stack frame when the recursive call is the final action in a function (a tail call), making recursion as memory-efficient as iteration.