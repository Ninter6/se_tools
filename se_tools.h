/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-08-22 18:55:03
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-02-03 04:21:09
 */
//
//  se_tools.h
//  nova
//
//  Created by Ninter6 on 2023/8/21.
//

#pragma once

#define LOOP(n) for (int i = 0; i < n; i++)
#define LOOPX(n, x) for (int x = 0; x < n; x++)
#define LOOPB(n, b) for (int i = b; i < n; i++)
#define LOOPBX(n, b, x) for (int x = b; x < n; x++)

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}
