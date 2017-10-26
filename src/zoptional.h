#ifndef ZOPTIONAL_H
#define ZOPTIONAL_H

#if __GNUC__ && (__GNUC__ < 7)
#include <experimental/optional>
namespace ZMB {
template<typename T>
using optional = std::experimental::optional<T>;
}
#elif
#include <optional>
namespace ZMB {
template<typename T>
using optional = std::optional<T>;
}
#endif
#endif // ZOPTIONAL_H
