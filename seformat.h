/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-10-21 13:21:34
 * @LastEditors: Ninter6
 * @LastEditTime: 2023-10-22 12:00:05
 */
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace st {

class FormatParser {
public:
    FormatParser(std::string_view fmt) : fmt(fmt) {
        parse_plchs();
    }

    template<class...Args, size_t...I>
    void parse(std::tuple<Args...> args, std::index_sequence<I...>) {
        ((parse_each<I>(args))...);
    }

private:
    std::string_view fmt;
    std::unordered_multimap<size_t/*index*/, std::pair<size_t/*pos*/, size_t/*length*/>> plchs;
    std::stringstream context;

private:
    void parse_plchs() {
        for (size_t i = fmt.find('{'); i < fmt.size(); i = fmt.find('{', i+1)) {
            if (fmt[i+1] == '{') {
                ++i;
                continue;
            }

            plchs.emplace(parse_index(i+1), std::make_pair(i, fmt.find('}', i) - i + 1ul));
        }
    }

    size_t parse_index(size_t pos) {
        if (std::isdigit(fmt[pos])) {
            auto bg = pos;
            pos = std::distance(std::begin(fmt), std::find_if_not(std::begin(fmt) + pos, std::end(fmt), [](auto&&c){return std::isdigit(c);}));
            return std::stoi(fmt.substr(bg, pos - bg).data());
        }
        return plchs.size();
    }
};

auto format(std::string_view fmt_str, auto&& var...) {
    FormatParser parser{fmt_str};
    
    parser.parse(std::forward_as_tuple<decltype(var)...>(var),
        std::index_sequence_for<decltype(var)...>{});
}

}