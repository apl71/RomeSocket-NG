#include <iostream>
#include <format>
#include <chrono>

namespace rome::detail {

inline std::string current_time() {
    using namespace std::chrono;

    const auto now = system_clock::now();

    const auto milliseconds = duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    const std::time_t time = system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&time, &tm);

    return std::format(
        "[{:04}-{:02}-{:02} "
        "{:02}:{:02}:{:02}.{:03}]",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        milliseconds.count()
    );
}

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