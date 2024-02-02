/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-10-21 13:21:34
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-02-03 02:50:55
 */
#pragma once

#include <vector>
#include <unordered_map>
#include <regex>
#include <numeric>

#if __cplusplus > 201703L
#define ST_CFUNC constexpr
#else
#define ST_CFUNC inline
#warning "seformat may not compilable under standards lower than C++20"
#endif

#define __format_throw throw std::invalid_argument("format error")

namespace st {

namespace str_process {

constexpr bool is_digit(char c) {
    return c>='0' && c<='9';
}

constexpr bool is_upper(char c) {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_lower(char c) {
    return c >= 'a' && c <= 'z';
}

constexpr char to_upper(char c) {
    return c - 32;
}

constexpr char to_lower(char c) {
    return c + 32;
}

ST_CFUNC std::string to_upper(std::string_view str) {
    std::string res(str.size(), '\0');
    std::transform(str.begin(), str.end(), res.begin(), [](auto&& c){return is_lower(c) ? to_upper(c) : c;});
    return res;
}

ST_CFUNC std::string to_lower(std::string_view str) {
    std::string res(str.size(), '\0');
    std::transform(str.begin(), str.end(), res.begin(), [](auto&& c){return is_upper(c) ? to_lower(c) : c;});
    return res;
}

enum class alignmode {
    right,
    left,
    middle
};

ST_CFUNC std::string str_align(std::string_view bg, std::string_view str, alignmode align) {
    if (bg.length() <= str.length()) return {str.data(), str.size()};

    std::string res{bg.begin(), bg.end()};
    switch (align) {
    case alignmode::right:
        std::copy(str.rbegin(), str.rend(), res.rbegin());
        break;

    case alignmode::left:
        std::copy(str.begin(), str.end(), res.begin());
        break;

    case alignmode::middle:
        int b = (res.size() - str.size()) / 2;
        std::copy(str.begin(), str.end(), res.begin() + b);
        break;
    }
    return res;
}

enum class radix {
    decimal,
    hex,
    octal,
    binary,
};

static const std::string num_map = "0123456789ABCDEF";

ST_CFUNC std::string dtos(long num, uint32_t rdx = 10) {
    if (num == 0) return "0";
    if (num < 0) return '-' + dtos(-num, rdx);
    std::string res;
    while (num > 0) {
        res.insert(res.begin(), num_map[num % rdx]);
        num /= rdx;
    }
    return res;
}
ST_CFUNC std::string dtos(long num, radix rdx) {
    switch (rdx) {
    default:
    case radix::decimal:
        return dtos(num, 10);
    case radix::hex:
        return "0X" + dtos(num, 16);
    case radix::octal:
        return '0' + dtos(num, 8);
    case radix::binary:
        return "0B" + dtos(num, 2);
    }
}

constexpr long ctod(char c) {
    if (is_digit(c)) return c - '0';
    if (is_upper(c)) return c - 'A' + 10;
    if (is_lower(c)) return c - 'a' + 10;
    return 0;
}

ST_CFUNC long stod(std::string_view str, int rdx = 10) {
    if (str.front() == '-') {
        str.remove_prefix(1);
        return -stod(str, rdx);
    }
    long res = 0, b = 1;
    while (!str.empty()) {
        auto n = ctod(str.back());
        res += (n < rdx ? n : 0) * b;
        b *= rdx;
        str.remove_suffix(1);
    }
    return res;
}

ST_CFUNC long stod(std::string_view str, radix rdx) {
    switch (rdx) {
    case radix::decimal:
        return stod(str, 10);
    case radix::hex:
        return stod(str, 16);
    case radix::octal:
        return stod(str, 8);
    case radix::binary:
        return stod(str, 2);
    }
}

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
ST_CFUNC std::string ftos(T value, uint32_t pre) {
    std::string result;
    if (value < 0) {
        result.push_back('-');
        value = -value;
    }

    auto int_part = static_cast<int>(value);
    result += dtos(int_part);

    if (pre > 0) {
        result.push_back('.');
        auto frac_part = value - static_cast<T>(int_part);
        for (std::size_t i = 0; i < pre; ++i) {
            frac_part *= 10;
            int digit = static_cast<int>(frac_part);
            result.push_back(digit + '0');
            frac_part -= static_cast<T>(digit);
        }
    }

    return result;
}

ST_CFUNC std::string replace(std::string_view str, std::string_view r, std::string_view t) {
    std::string res;
    while (!str.empty()) {
        auto p = str.find(r);
        res.append(str.begin(), str.begin() + p);
        if (p != str.size()) res.append(t.begin(), t.end());
        str.remove_prefix(p + r.size());
    }
    return res;
}

struct fmt_opt {
    constexpr fmt_opt(std::string_view fmt) {
        for (int i = 0; i < fmt.size(); ++i) {
            switch (fmt[i]) {
            case '>':
                align = alignmode::right;
                break;
            case '<':
                align = alignmode::left;
                break;
            case '^':
                align = alignmode::middle;
                break;
            case '+':
                showpos = true;
                break;
            case '#':
                type = cv_type::integer;
                read_radix(fmt.back());
                fmt.remove_suffix(1);
                break;
            case '.': {
                type = cv_type::floating;
                int buf = 0;
                while (is_digit(fmt[i+1]))
                    buf = buf * 10 + fmt[++i] - '0';
                floating = buf;
                break;
            }
            case 'd':
                type = cv_type::integer;
                break;
            case 'f':
                type = cv_type::floating;
                break;
            case 'c':
                type = cv_type::character;
                break;
            case 's':
                type = cv_type::string;
                break;
            case ' ':
                break;
            default:
                if (is_digit(fmt[i])) {
                    int buf = fmt[i] - 48;
                    while (is_digit(fmt[i+1]))
                        buf = buf * 10 + fmt[++i] - 48;
                    width = buf;
                    break;
                }
                switch (fmt[i+1]) {
                case '>':
                    placeholder = fmt[i++];
                    align = alignmode::right;
                    break;
                case '<':
                    placeholder = fmt[i++];
                    align = alignmode::left;
                    break;
                case '^':
                    placeholder = fmt[i++];
                    align = alignmode::middle;
                    break;
                default:
                    __format_throw;
                    break;
                }
            }
        }
    }

    constexpr void read_radix(char c) {
        switch (c) {
        case 'x':
        case 'X':
            rdx = radix::hex;
            break;
        case '0':
            rdx = radix::octal;
            break;
        case 'b':
        case 'B':
            rdx = radix::binary;
            break;
        default:
            __format_throw;
            break;
        }
    }

    enum class cv_type {
        unknow,
        integer,
        floating,
        character,
        string
    };
    uint32_t width = 0, floating = 3;
    bool showpos = false;
    char placeholder = ' ';
    alignmode align = alignmode::right;
    radix rdx = radix::decimal;
    cv_type type = cv_type::unknow;
};

}

#define SPRC_ str_process::

template <class T, class = void>
struct Formatter;

class FormatParser {
public:
    FormatParser(std::string_view fmt) {
        parse_context({fmt.data(), fmt.size()});
    }

    /**
     * this function is disposable
     */
    std::string execute(auto&&... args) {
        parse(std::index_sequence_for<decltype(args)...>{},
              std::forward<decltype(args)>(args)...);
        return std::accumulate(context.begin(), context.end(), std::string{});
    }

    std::string operator()(auto&&... args) const {
        auto context_backup = context;
        parse(std::index_sequence_for<decltype(args)...>{},
              std::forward<decltype(args)>(args)...);
        context.swap(context_backup);
        return std::accumulate(context_backup.begin(), context_backup.end(), std::string{});
    }

    template <class array_t>
    std::string parse_array(array_t&& array) const {
        auto context_backup = context;
        for (size_t i = 0; auto&& f : array)
            parse_each(i++, std::forward<decltype(f)>(f));
        context.swap(context_backup);
        return std::accumulate(context_backup.begin(), context_backup.end(), std::string{});
    }

    template<class...Args, size_t...I>
    void parse(std::index_sequence<I...>, Args&&... args) const {
        ((parse_each(I, std::forward<Args>(args))), ...);
    }

    size_t num_args() const {
        size_t n = 0;
        for (const auto& [i, _] : holders)
            if (i + 1 > n)
                n = i + 1;
        return n;
    }

private:
    mutable std::vector<std::string> context;
    std::unordered_multimap<size_t/*index*/, size_t/*pos of context*/> holders;

private:
    void parse_context(std::string remaining) {
        std::regex pattern(R"(\{([^\{\}\\]*)\})"), remove(R"(\\\})");  // 匹配{}内的内容
    
        std::smatch match;
    
#define REPLACE(str) std::regex_replace(str, remove, "}")
        while (std::regex_search(remaining, match, pattern)) {
            // 匹配到的部分
            const std::string& part1 = match.prefix();  // {}左边的部分
            const std::string& part2 = match[1];         // {}内的部分
            if (!part1.empty()) context.push_back(REPLACE(part1));
            context.push_back(REPLACE(part2));
    
            holders.emplace(parse_index(), context.size()-1);
    
            remaining = match.suffix();
        }
    
        if (!remaining.empty()) {
            context.push_back(REPLACE(remaining));
        }
#undef REPLACE
    }

    size_t parse_index() {
        std::string str = context.back().substr(0, context.back().find(':'));
        std::stable_partition(str.begin(), str.end(), SPRC_ is_digit);
        if (!str.empty())
            return SPRC_ stod(str);

        return holders.size();
    }

    template <class T>
    void parse_each(size_t I, T&& arg) const {
        auto [b, e] = holders.equal_range(I);
        for (auto it = b; it != e; ++it) {
            auto& str = context[it->second];
            std::string_view fmt{str};
            if (auto fi = fmt.find(':'); fi + 1 < fmt.size())
                 fmt = fmt.substr(fi + 1);
            else fmt = {};
            str = Formatter<std::remove_cvref_t<T>>{fmt}(std::forward<T>(arg));
        }
    }
};

struct OneOffFormatParser {
    OneOffFormatParser(std::string_view fmt) : fmt(fmt) {}

    std::string operator()(auto&&... args) {
        return fmt.execute(std::forward<decltype(args)>(args)...);
    }

    FormatParser fmt;
};

auto format(std::string_view fmt_str, auto&&... var) {
    OneOffFormatParser parser{fmt_str};
    return parser(std::forward<decltype(var)>(var)...);
}

namespace format_literal {
    inline auto operator""_f(const char* cstr, size_t size) {
        return OneOffFormatParser{{cstr, size}};
    }
}

template <>
struct Formatter<bool> {
    Formatter(std::string_view fmt) : opt(fmt) {};
    std::string operator()(bool c) {
        std::string buf{};

        switch (opt.type) {
        case SPRC_ fmt_opt::cv_type::integer:
            buf = c ? "1" : "0";
            break;
        default:
            buf = c ? "true" : "false";
            break;
        }

        return SPRC_ str_align(std::string(opt.width, opt.placeholder), buf, opt.align);
    }

    SPRC_ fmt_opt opt;
};

template <>
struct Formatter<char> {
    Formatter(std::string_view fmt) : opt(fmt) {};
    std::string operator()(char c) {
        std::string buf{};

        switch (opt.type) {
        case SPRC_ fmt_opt::cv_type::integer:
            buf = SPRC_ dtos(c, opt.rdx);
            break;
        default:
            buf = c;
            break;
        }

        return SPRC_ str_align(std::string(opt.width, opt.placeholder), buf, opt.align);
    }

    SPRC_ fmt_opt opt;
};

template <class T, class = void>
struct is_string : std::false_type {};

template <class T>
struct is_string<T, std::enable_if_t<std::is_constructible_v<std::string_view, T>>> : std::true_type {};

template <class T>
struct Formatter<T, std::enable_if_t<is_string<T>::value>> {
    Formatter(std::string_view fmt) : opt(fmt) {};
    std::string operator()(std::string_view str) {return SPRC_ str_align(std::string(opt.width, opt.placeholder), str, opt.align);}
private:
    SPRC_ fmt_opt opt;
};

template <class T>
struct Formatter<T, std::enable_if_t<std::is_integral_v<T>>> {
    Formatter(std::string_view fmt) : opt(fmt) {}
    std::string operator()(T n) {
        std::string buf{};

        switch (opt.type) {
        case SPRC_ fmt_opt::cv_type::unknow:
        case SPRC_ fmt_opt::cv_type::integer:
            buf = SPRC_ dtos(n, opt.rdx);
            break;
        default:
            __format_throw;
            break;
        }

        return SPRC_ str_align(std::string(opt.width, opt.placeholder), buf, opt.align);
    }

private:
    SPRC_ fmt_opt opt;

};

template <class T>
struct Formatter<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    Formatter(std::string_view fmt) : opt(fmt) {}
    std::string operator()(T n) {
        std::string buf{};

        switch (opt.type) {
        case SPRC_ fmt_opt::cv_type::unknow:
        case SPRC_ fmt_opt::cv_type::floating:
            buf = SPRC_ ftos(n, opt.floating);
            break;
        default:
            __format_throw;
            break;
        }
        if (opt.showpos && n > 0) buf = '+' + buf;

        return SPRC_ str_align(std::string(opt.width, opt.placeholder), buf, opt.align);
    }

private:
    SPRC_ fmt_opt opt;

};

#define _to_string(x) Formatter<std::remove_cvref_t<decltype(x)>>{{}}((x));

template <class T1, class T2>
struct Formatter<std::pair<T1, T2>> {
    Formatter(std::string_view fmt) {
        if (fmt.size() == 1)
            switch (fmt[0]) {
            case 'k':
                md = mode::key;
                break;
            case 'v':
                md = mode::value;
                break;
            default:
                break;
            }
    }
    std::string operator()(const std::pair<T1, T2>& p) {
        switch (md) {
        case mode::key:
            return _to_string(p.first);
            break;
        case mode::value:
            return _to_string(p.second);
            break;
        default:
            return FormatParser{"{}:{}"}(p.first, p.second);
            break;
        }
    }

private:
    enum class mode {
        both,
        key,
        value
    };
    mode md = mode::both;
};

template <class T, class = void>
struct is_iterable : std::false_type {};

template <class T>
struct is_iterable<T, typename std::iterator_traits<decltype(std::begin(std::declval<T&>()))>::value_type> : std::true_type {};

template <class T, size_t N>
struct is_iterable<T[N]> : std::true_type {};

template <class T>
struct is_formatable_range {
    static constexpr bool value = !is_string<T>::value && is_iterable<T>::value;
};

template <class T>
struct Formatter<T, std::enable_if_t<is_formatable_range<T>::value>> {
    Formatter(std::string_view){};
    std::string operator()(const T& t){
        std::string res = "{";
        for (auto it = std::begin(t);;) {
            res += _to_string(*it);
            if (++it != std::end(t)) res += ", ";
            else {res += '}'; break;}
        }
        return res;
    }
};

#undef ST_CFUNC
#undef __format_throw
#undef SPRC_
#undef _to_string

}