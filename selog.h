/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-11-17 22:33:05
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-02-19 03:29:29
 */
#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <source_location>

#include "seformat.h"

#define FMT st::format

namespace st::log {

#define FOREACH_LOG_LEVEL(f) \
    f(trace) \
    f(debug) \
    f(info) \
    f(critical) \
    f(warning) \
    f(error) \
    f(fatal) 

enum class log_level {
#define _func(x) x,
    FOREACH_LOG_LEVEL(_func)
#undef _func
};

#ifndef SELOG_DISABLE_ANSI
#define     LEVEL_COLOR_trace       "\E[37m"
#define     LEVEL_COLOR_debug       "\E[35m"
#define     LEVEL_COLOR_info        "\E[32m"
#define     LEVEL_COLOR_critical    "\E[34m"
#define     LEVEL_COLOR_warning     "\E[33m"
#define     LEVEL_COLOR_error       "\E[31m"
#define     LEVEL_COLOR_fatal       "\E[31;1m"
#define     LEVEL_COLOR_CLEAR       "\E[m"
#define     LEVEL_COLOR(x)          LEVEL_COLOR_##x
#else
#define     LEVEL_COLOR_CLEAR
#define     LEVEL_COLOR(x)
#endif

constexpr const char* log_level_name(log_level lev) {
#define _func(x) case log_level:: x : return LEVEL_COLOR(x) #x LEVEL_COLOR_CLEAR;
    switch (lev) {
    FOREACH_LOG_LEVEL(_func)
    default:
        assert(false && "Unkonw level!");
    }
#undef _func
}

namespace {
std::ostream* op_stream = &std::cout;

#ifdef NDEGUG
log_level min_lev = log_level::info;
#else 
log_level min_lev = log_level::trace;
#endif

std::string time_format = "[ %x - %X ]:"; // dont support yet
} // global variable

inline void set_min_Level(log_level min) {
    min_lev = min;
}
inline log_level get_min_Level() {
    return min_lev;
}

inline void set_time_format(const std::string& tfmt) {
    time_format = tfmt;
}

inline void open_file(const std::string& filename) {
    if (op_stream != &std::cout) delete op_stream;
    op_stream = new std::ofstream(filename, std::ios::app);
}

inline void use_stdout() {
    if (op_stream == &std::cout) return;
    delete op_stream;
    op_stream = &std::cout;
}

struct printer {
    constexpr printer(const char* el) : el(el) {}
    template <class...Args>
    void operator()(std::string_view fmt, Args...args) const {
        (*op_stream) << FMT(fmt, std::forward<Args>(args)...) << el;
    }
    void operator()(std::string_view str) const {
        (*op_stream) << str << el;
    }
    const char* el;
};
constexpr printer print{""};
constexpr printer print_ln{"\n"};
constexpr printer print_tab{"\n\t"};

template <class T>
struct with_source_localtion {
    template <class U, 
              class = std::enable_if_t<std::is_constructible_v<T, U>>>
    constexpr with_source_localtion(U inner, std::source_location loc = std::source_location::current())
    : inner(std::forward<U>(inner)), location(loc) {}

    constexpr T& get() {return inner;}
    constexpr const T& get() const {return inner;}
    
    T inner;
    std::source_location location;
};

template <class...Args>
void titled_log(std::string_view title, std::string_view fmt, Args...args) {
    auto fmt_str = "[{}]: " + std::string(fmt.data(), fmt.size());
    print_ln(fmt_str, title, std::forward<Args>(args)...);
}

template <class...Args>
void location_log(with_source_localtion<std::string_view> title, std::string_view fmt, Args...args) {
    print_tab("{}:{} in {}:", title.location.file_name(), title.location.line(), title.location.function_name());
    titled_log(title.get(), fmt, std::forward<Args>(args)...);
}

template <class...Args>
void location_log(with_source_localtion<log_level> lev, std::string_view fmt, Args...args) {
    if (lev.get() < min_lev) return;
    print_tab("{}:{} in {}:", lev.location.file_name(), lev.location.line(), lev.location.function_name());
    titled_log(log_level_name(lev.get()), fmt, std::forward<Args>(args)...);
}

template <class...Args>
void time_log(std::string_view title, std::string_view fmt, Args...args) {
    // std::chrono::zoned_time now{std::chrono::current_zone(), std::chrono::high_resolution_clock::now()};
    // (*op_stream) << "[ " << now << " ]:" << "\n\t";
    // titled_log(title, fmt, std::forward<Args>(args)...);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto tm = std::localtime(&now);
    (*op_stream) << std::put_time(tm, time_format.c_str()) << "\n\t";
    titled_log(title, fmt, std::forward<Args>(args)...);
}

template <class...Args>
void time_log(log_level lev, std::string_view fmt, Args...args) {
    if (lev < min_lev) return;
    // std::chrono::zoned_time now{std::chrono::current_zone(), std::chrono::high_resolution_clock::now()};
    // (*op_stream) << "[ " << now << " ]:" << "\n\t";
    // titled_log(log_level_name(lev), fmt, std::forward<Args>(args)...);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto tm = std::localtime(&now);
    (*op_stream) << std::put_time(tm, time_format.c_str()) << "\n\t";
    titled_log(log_level_name(lev), fmt, std::forward<Args>(args)...);
}

#define _func(x) template <class...Args> \
    void log_##x(std::string_view fmt, Args...args) {if (log_level::x < min_lev) return; titled_log(log_level_name(log_level::x), fmt, std::forward<Args>(args)...);}
    FOREACH_LOG_LEVEL(_func)
#undef _func

#define _func(x) template <class...Args> \
    void location_##x(with_source_localtion<std::string_view> fmt, Args...args) {location_log({log_level::x, fmt.location}, fmt.get(), std::forward<Args>(args)...);}
    FOREACH_LOG_LEVEL(_func)
#undef _func

#define _func(x) template <class...Args> \
    void time_##x(std::string_view fmt, Args...args) {time_log(log_level::x, fmt, std::forward<Args>(args)...);}
    FOREACH_LOG_LEVEL(_func)
#undef _func

}

#undef FMT
#undef FOREACH_LOG_LEVEL
