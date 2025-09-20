#ifndef HYPERTK_COMMON_HPP
#define HYPERTK_COMMON_HPP

/** @brief Enable compiler optimization pass */
#define ENABLE_COMPILER_OPTIMIZATION_PASS

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