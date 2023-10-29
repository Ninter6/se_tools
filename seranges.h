/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-10-15 01:39:42
 * @LastEditors: Ninter6
 * @LastEditTime: 2023-10-28 22:00:33
 */
#pragma once

#include <ranges>
#include <iterator>
#include <utility>

using std::begin;
using std::end;
using std::cbegin;
using std::cend;
using std::rbegin;
using std::rend;
using std::crbegin;
using std::crend;

namespace st {

namespace ranges {

template <class T, class = void>
struct is_range : std::false_type {};

template <class T>
struct is_range<T, std::void_t<decltype(begin(std::declval<T>()), end(std::declval<T>()))>> : std::true_type {};

template <class T>
constexpr bool is_range_v = is_range<T>::value;

template <class T>
using iterator_t = std::remove_reference_t<decltype(begin(std::declval<T>()))>;

template <class T>
using iterator_value_t = typename std::iterator_traits<T>::value_type;

template <class T>
using range_value_t = iterator_value_t<iterator_t<T>>;

auto size(auto&& range)
-> std::enable_if_t<is_range_v<decltype(range)>, size_t> {
    return std::distance(begin(range), end(range));
}

auto empty(auto&& range)
-> std::enable_if_t<is_range_v<decltype(range)>, bool> {
    return size(range) == 0;
}

auto data(auto&& range)
-> std::enable_if_t<is_range_v<decltype(range)>, range_value_t<decltype(range)>*> {
    return std::data(range);
}

auto cdata(auto&& range)
-> std::enable_if_t<is_range_v<decltype(range)>, range_value_t<decltype(range)>*> {
    return std::as_const(std::data(range));
}

auto for_each(auto&& range, auto&& func)
-> std::enable_if_t<std::is_invocable_v<decltype(func),  range_value_t<decltype(range)>>, decltype((func))> {
    for (auto&& i : range) func(i);
    return func;
}

auto transform(auto&& range, auto&& func)
-> std::enable_if_t<
    !std::is_const_v<range_value_t<decltype(range)>> &&
    std::is_invocable_v<decltype(func),  range_value_t<decltype(range)>> &&
    std::is_assignable_v<range_value_t<decltype(range)>, decltype(func(*begin(range)))>, 
    decltype((func))> {
    for (auto&& i : range) i = func(i);
    return func;
}

auto find(auto&& range, auto&& val)
-> decltype(*begin(range) == val, begin(range)) {
    auto it = begin(range);
    for(;it != end(range); ++it)
        if (*it == val)
            break;
    return it;
}

auto find_end(auto&& range, auto&& val)
-> decltype(*rbegin(range) == val, rbegin(range)) {
    auto it = rbegin(range);
    for(;it != rend(range); ++it)
        if (*it == val)
            break;
    return it;
}

auto find_if(auto&& range, auto&& func)
-> decltype(!func(*begin(range)), begin(range)) {
    auto it = begin(range);
    for(;it != end(range); ++it)
        if (func(*it))
            break;
    return it;
}

auto find_if_not(auto&& range, auto&& func)
-> decltype(!func(*begin(range)), begin(range)) {
    auto it = begin(range);
    for(;it != end(range); ++it)
        if (!func(*it))
            break;
    return it;
}

auto copy(auto&& range, auto&& opIt)
-> decltype(*opIt = *begin(range), opIt) {
    for (const auto& i : range)
        *opIt++ = i;
    return opIt;
}

auto copy_if(auto&& range, auto&& opIt, auto&& func)
-> decltype(*opIt = *begin(range), func(*begin(range)), opIt) {
    for (const auto& i : range)
        if(func(i))
            *opIt++ = i;
    return opIt;
}

auto copy_n(auto&& range, size_t n, auto&& opIt)
-> decltype(*opIt = *begin(range), opIt) {
    for (size_t I = 0; const auto& i : range) {
        if (I++ == n) break;
        *opIt++ = i;
    }
    return opIt;
}

auto copy_backward(auto&& range, auto&& opIt)
-> decltype(*opIt = *rbegin(range), opIt) {
    for(auto it = rbegin(range); it != rend(range); ++it)
        *opIt++ = *it;
    return opIt;
}

auto move(auto&& range, auto&& opIt)
-> std::enable_if_t<
    std::is_nothrow_move_constructible_v<range_value_t<decltype(range)>> &&
    std::is_nothrow_move_assignable_v<range_value_t<decltype(range)>>,
    std::remove_reference_t<decltype(opIt)>> {
    for (auto it = std::make_move_iterator(begin(range)); it != std::make_move_iterator(end(range)); ++it)
        *opIt++ = *it;
    return opIt;
}

auto move_backward(auto&& range, auto&& opIt)
-> std::enable_if_t<
    std::is_nothrow_move_constructible_v<range_value_t<decltype(range)>> &&
    std::is_nothrow_move_assignable_v<range_value_t<decltype(range)>>,
    std::remove_reference_t<decltype(opIt)>> {
    for (auto it = std::make_move_iterator(rbegin(range)); it != std::make_move_iterator(rend(range)); ++it)
        *opIt++ = *it;
    return opIt;
}

auto fill(auto&& range, auto&& val)
-> decltype(*begin(range) = val, void{}) {
    for (auto&& i : range) i = val;
}

auto fill_n(auto&& range, size_t n, auto&& val)
-> decltype(*begin(range) = val, void{}) {
    for (size_t I = 0; auto&& i : range)
        if (I++ != n)
            i = val;
        else break;
}

auto generate(auto&& range, auto&& func)
-> decltype(*begin(range) = func(), void{}) {
    for (auto&& i : range) i = func();
}

auto generate(auto&& range, auto&& func)
-> decltype(*begin(range) = func(size_t{}), void{}) {
    for (size_t I = 0; auto&& i : range) i = func(I++);
}

auto generate_n(auto&& range, size_t n, auto&& func)
-> decltype(*begin(range) = func(), void{}) {
    for (size_t I = 0; auto&& i : range)
        if (I++ != n)
            i = func();
        else break;
}

auto generate_n(auto&& range, size_t n, auto&& func)
-> decltype(*begin(range) = func(n), void{}) {
    for (size_t I = 0; auto&& i : range)
        if (I != n)
            i = func(I++);
        else break;
}

}

}