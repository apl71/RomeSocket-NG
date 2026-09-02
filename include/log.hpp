#include <iostream>
#include <format>

#include "utils.hpp"

namespace rome::detail {

template <typename... Args>
void rome_log(std::format_string<Args...> format, Args&&... args) {
    std::cerr << current_time() << " "
              << std::format(format, std::forward<Args>(args)...) << '\n';
}

#define ROME_ENABLE_LOG

#ifdef ROME_ENABLE_LOG

#define ROME_LOG(...) \
    rome::detail::rome_log(__VA_ARGS__)

#else

#define ROME_LOG(...) \
    (void(0))

#endif

}