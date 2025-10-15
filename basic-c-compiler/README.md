#Overview

Basic C compiler based on [A Compiler Writing Journey](https://github.com/DoctorWkt/acwj) for learning about compiler backend. My compilers have backend for x86-64 and AArch64 (MacOS Apple Silicon) and is only tested on Ubuntu 22.04 x86-64 and Mac M4 pro.

## Note on limitations

### Variadic function

Currently my compiler only support for parsing prototype of variadic function or invoking variadic function. Compiler does not handle variadic parameters for variadic function implementation.
