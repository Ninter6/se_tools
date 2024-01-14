#pragma once

#include <iostream>
#include <fstream>
#include <cassert>
#include <source_location>

#include "seformat.h"

#define FMT st::format

namespace st::log {

namespace {
std::ostream* op_stream = &std::cout;
} // global variable

inline void open_file(const std::string& filename) {
    if (op_stream != &std::cout) delete op_stream;
    op_stream = new std::ofstream(filename);
}

struct printer {
    constexpr printer() = default;
    template <class...Args>
    void operator()(Args...args) const {
        (((*op_stream) << FMT("{}", args)), ...);
        std::endl(std::cout);
    }
};
constexpr printer print;

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

constexpr const char* log_level_name(log_level lev) {
#define _func(x) case log_level:: x : return #x;
    switch (lev) {
    FOREACH_LOG_LEVEL(_func)
    default:
        assert(false && "Unkonw level!");
    }
#undef _func
}

template <class...Args>
void titled_log(const std::string& title, Args...args) {
    print('['+title+"]: ", std::forward<Args>(args)...);
}

template <class...Args>
void location_log(with_source_localtion<std::string_view> title, Args...args) {
    print(FMT("{}:{}: {}:", title.location.file_name(), title.location.line(), title.get()));
    print(args...);
}

#define _func(x) template <class...Args> \
    void log_##x(Args...args) {titled_log(#x, std::forward<Args>(args)...);}
    FOREACH_LOG_LEVEL(_func)
#undef _func

}

#undef FMT

#ifdef NDEGUG
#define     DEBUG_LOG
#else
#define     DEBUG_LOG(e) do{std::cerr<<e<<std::endl;}while(0)
#endif
