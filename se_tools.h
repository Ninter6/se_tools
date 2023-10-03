//
//  se_tools.h
//  nova
//
//  Created by Ninter6 on 2023/8/21.
//

#pragma once

#include <iostream>

#include "mathpls.h"

#include "setimer.h"
#include "sejson.h"
#include "sebase64.h"
#include "secompress.h"
#include "seargs.h"

#ifdef NDEGUG

#define DEBUG_LOG

#else

#define DEBUG_LOG(e) do{std::cerr<<e<<std::endl;}while(0)

#endif

#define LOOP(n) for (int i = 0; i < n; i++)
#define LOOPX(n, x) for (int x = 0; x < n; x++)
#define LOOPB(n, b) for (int i = b; i < n; i++)
#define LOOPBX(n, b, x) for (int x = b; x < n; x++)

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

// random

#include <glm/glm.hpp>

namespace st::random {

using namespace mathpls::random;

inline glm::vec3 randv3() {
    return {mathpls::random::rand(), mathpls::random::rand(), mathpls::random::rand()};
}

inline glm::vec3 randv3_01() {
    return {rand01(), rand01(), rand01()};
}

inline glm::vec3 randv3_11() {
    return {rand11(), rand11(), rand11()};
}

}
