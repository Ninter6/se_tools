/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-10-21 13:21:34
 * @LastEditors: Ninter6
 * @LastEditTime: 2023-10-29 11:52:31
 */
#pragma once

#include <vector>
#include <tuple>
#include <unordered_map>
#include <regex>
#include <numeric>

namespace st {

template <class T>
struct Formatter {
    Formatter(std::string_view){};
    virtual std::string operator()(T) = 0; // unrealized
};

class FormatParser {
public:
    FormatParser(std::string_view fmt) {
        parse_context({fmt.data(), fmt.size()});
    }

    std::string operator()(auto&&... args) noexcept {
        parse(std::forward_as_tuple<decltype(args)...>(args...),
            std::index_sequence_for<decltype(args)...>{});
        return std::accumulate(context.begin(), context.end(), std::string{});
    }

    template<class...Args, size_t...I>
    void parse(std::tuple<Args...> args, std::index_sequence<I...>) {
        ((parse_each<I>(std::get<I>(args))), ...);
    }

public:
    std::vector<std::string> context;
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
        const std::string& str = context.back().substr(0, context.back().find(':'));
        if (!str.empty())
            try {
                return std::stoul(str);
            } catch(const std::exception& e) {
                throw std::invalid_argument("Illegal formatting parameters: " + context.back());
            }

        return holders.size();
    }

    template <size_t I, class T>
    void parse_each(T arg) {
        auto [b, e] = holders.equal_range(I);
        for (auto it = b; it != e; ++it) {
            auto& str = context[it->second];
            std::string_view fmt = str.find(':') < str.size() ? str.substr(str.find(':')) : "";
            str = Formatter<std::remove_reference_t<std::remove_cv_t<T>>>{fmt}(std::forward<T>(arg));
        }
    }
};

auto format(std::string_view fmt_str, auto&&... var) {
    FormatParser parser{fmt_str};
    return parser(std::forward<decltype(var)>(var)...);
}

namespace format_literal {
    auto operator""_f(const char* cstr, size_t size) {
        return FormatParser{{cstr, size}};
    }
}

template <>
struct Formatter<const char*> {
    Formatter(std::string_view){};
    std::string operator()(const char* cstr) {return cstr;}
};
template <>
struct Formatter<std::string> {
    Formatter(std::string_view){};
    std::string operator()(const std::string& str) {return str;}
};
template <>
struct Formatter<std::string_view> {
    Formatter(std::string_view){};
    std::string operator()(std::string_view str) {return {str.data(), str.size()};}
};

}