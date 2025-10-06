# Note about some issue

### Issue when allocating global variable with array type

- Currently, this compiler treat array as contiguous memory area of pointer type instead of value type. For example, `int a[5]` will be alloc with `.quad` (8 bytes) instead of `.long` (4 bytes). But the array access operator will scale idx by size of int (4 bytes) instead of 8 bytes. This is a wrong behavior but it still output correct. It must be fix later.
