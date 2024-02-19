/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-08-22 18:55:03
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-02-03 13:08:54
 */
//
//  se_tools.h
//  nova
//
//  Created by Ninter6 on 2023/8/21.
//

#pragma once

#include "selog.h"

#define LOOP(n) for (int i = 0; i < n; i++)
#define LOOPX(n, x) for (int x = 0; x < n; x++)
#define LOOPB(n, b) for (int i = b; i < n; i++)
#define LOOPBX(n, b, x) for (int x = b; x < n; x++)

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

#define TRACE_LOG(...) st::log::log_trace(__VA_ARGS__)

#define DEBUG_LOG(...) st::log::log_debug(__VA_ARGS__)

#define INFO_LOG(...) st::log::log_info(__VA_ARGS__)

#define CRITICAL_LOG(...) st::log::log_critical(__VA_ARGS__)

#define WARNING_LOG(...) st::log::location_warning(__VA_ARGS__)

#define ERROR_LOG(...) do{st::log::location_error(__VA_ARGS__); \
    throw std::runtime_error(__FUNCTION__);}while(0)

#define FATAL_LOG(...) do{st::log::location_fatal(__VA_ARGS__); \
    std::terminate();}while(0)

#define SE_STR(x) #x
#define SE_XSTR(x) SE_STR(x)

#define CREF(x) const x&
