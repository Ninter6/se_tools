#pragma once

#include <string>
#include <vector>
#include <map>

namespace st {

namespace base64 {

static const std::string base64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/"; // Base64字符集

inline std::string encode(const std::string& input) {
    std::string encoded;
    int val = 0;
    int bits = -6;

    for (unsigned char c : input) {
        val = (val << 8) + c;
        bits += 8;

        while (bits >= 0) {
            encoded.push_back(base64Chars[(val >> bits) & 63]);
            bits -= 6;
        }
    }

    if (bits > -6) {
        encoded.push_back(base64Chars[((val << 8) >> (bits + 8)) & 63]);
    }

    // 添加填充字符 "="
    while (encoded.size() % 4 != 0) {
        encoded.push_back('=');
    }

    return encoded;
}

inline std::string decode(const std::string& input) {
    std::string decoded;
    int val = 0;
    int bits = -8;

    for (unsigned char c : input) {
        if (c == '=')
            break;
        
        val = (val << 6) + base64Chars.find(c);
        bits += 6;

        if (bits >= 0) {
            decoded.push_back((val >> bits) & 255);
            bits -= 8;
        }
    }

    return decoded;
}

}

namespace base58 {

// Base58字符集
std::string base58Chars =
    "123456789"
    "ABCDEFGHJKLMNPQRSTUVWXYZ"
    "abcdefghijkmnopqrstuvwxyz";

inline std::string encode(const std::string& input) {
    std::vector<int> tmp(input.size() * 138 / 100 + 1);
    for (int i = 0; i < input.size(); i++)
        for (int j = tmp.size() - 1, cur = input[i]; cur; j--)
            tmp[j] = (cur += tmp[j] << 8) % 58, cur /= 58;
    std::string res;
    for (int i = 0; i < tmp.size(); i++) {
        if (!tmp[i])continue;
        for (int j = i; j < tmp.size(); j++)
            res.push_back(base58Chars[tmp[j]]);
        break;
    }
    return res;
}

inline std::string decode(const std::string& input) {
    std::map<char, int> mp;
    for (int i = 0; i < base58Chars.size(); i++)mp[base58Chars[i]] = i;
    std::vector<int> tmp;
    for (int i = 0; i < input.size(); i++) {
        int cur = mp[input[i]];
        for (int j = 0; j < tmp.size(); j++)
            tmp[j] = (cur += tmp[j] * 58) & 0xFF, cur >>= 8;
        while (cur) tmp.push_back(cur & 0xFF), cur >>= 8;
    }
    return {tmp.rbegin(), tmp.rend()};
}

}

namespace 原神 {

static std::string 原神启动[] = {"原", "神", "启", "动"};

inline std::string encode(const std::string& input) {
    std::string encoded;

    for (auto c : input)
        for (int i = 3; i >= 0; i--)
            encoded.insert(encoded.size(), 原神启动[(c >> i*2) & 3]);

    return encoded;
}

inline std::string decode(const std::string& input) {
    std::string decoded;
    uint8_t b = 0, n = 0;

    for (int i = 0; i < input.size(); i += 3) {
        const auto& c = input.substr(i, 3);
        for (int j = 0; j < 4; j++)
            if (c == 原神启动[j]) {
                b = (b << 2) | j;
                if (++n == 4) {
                    decoded.push_back(b);
                    b = n = 0;
                }
                break;
            }
    }

    return decoded;
}

}

}
