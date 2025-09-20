#ifndef HYPERTK_BUILTIN_HPP
#define HYPERTK_BUILTIN_HPP

#ifdef _WIN32
/// @note for Windows we need to actually export the functions because the dynamic symbol loader will use `GetProcAddress` to find the symbols.
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#define __HYPERTK_BUILTIN_FUNCTION extern "C" DLLEXPORT

/// @brief Putchar that takes a double and returns 0.
/// @note Mark it `extern "C"` so that the function name doesn’t get mangled.
__HYPERTK_BUILTIN_FUNCTION double putchard(double X);

/// @brief Printf that takes a double prints it as "%f\n", returning 0.
__HYPERTK_BUILTIN_FUNCTION double printd(double X);

#endif