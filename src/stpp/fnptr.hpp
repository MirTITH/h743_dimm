#include <type_traits>
#include <utility>
#include <new>

template <int, typename Callable, typename Ret, typename... Args>
auto fnptr_(Callable &&c, Ret (*)(Args...))
{
    static std::decay_t<Callable> storage = std::forward<Callable>(c);
    static bool used                      = false;
    if (used) {
        using type = decltype(storage);
        storage.~type();
        new (&storage) type(std::forward<Callable>(c));
    }
    used = true;

    return [](Args... args) -> Ret {
        auto &c = *std::launder(&storage);
        return Ret(c(std::forward<Args>(args)...));
    };
}

template <typename Fn, int N = 0, typename Callable>
Fn *fnptr(Callable &&c)
{
    return fnptr_<N>(std::forward<Callable>(c), (Fn *)nullptr);
}