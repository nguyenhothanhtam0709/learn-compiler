#ifndef HYPERTK_COMMON_HPP
#define HYPERTK_COMMON_HPP

/** @brief Enable compiler optimization pass */
#define ENABLE_COMPILER_OPTIMIZATION_PASS
/** @brief Enable print ast */
#define ENABLE_PRINTING_AST
/** @brief Enable semantic analyzing */
#define ENABLE_SEMATIC_ANALYZING
/** @brief Enable print llvm ir */
#define ENABLE_PRINTING_LLVM_IR
/** @brief Enable basic jit compiler */
#define ENABLE_BASIC_JIT_COMPILER
/** @brief Enable built-in functions */
#define ENABLE_BUILTIN_FUNCTIONS

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class Uncopyable
{
public:
    explicit Uncopyable(const Uncopyable &) = delete;
    Uncopyable &operator=(const Uncopyable &) = delete;

protected:
    Uncopyable() = default;
    virtual ~Uncopyable() = default;
};

#endif