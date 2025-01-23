#pragma once

#include <string>
#include <vector>

// 右移的时候，高位一定要补零，而不是补充符号位
#define shift(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

namespace detail {

//常量ti unsigned(abs(sin(i+1))*(2pow32))
constexpr unsigned k[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8,
    0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193,
    0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51,
    0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905,
    0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681,
    0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60,
    0xbebfbc70, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244,
    0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
    0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314,
    0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};
//向左位移数
constexpr unsigned s[] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7,
    12, 17, 22, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 6, 10,
    15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};
constexpr char str16[] = "0123456789abcdef";

inline void mainLoop(unsigned M[], unsigned& atemp, unsigned& btemp, unsigned& ctemp, unsigned& dtemp) {
    unsigned f, g;
    unsigned a = atemp;
    unsigned b = btemp;
    unsigned c = ctemp;
    unsigned d = dtemp;

    for (unsigned i = 0; i < 64; i++) {
        if (i < 16) {
            f = F(b, c, d);
            g = i;
        } else if (i < 32) {
            f = G(b, c, d);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = H(b, c, d);
            g = (3 * i + 5) % 16;
        } else {
            f = I(b, c, d);
            g = (7 * i) % 16;
        }
        unsigned tmp = d;
        d = c;
        c = b;

        b = b + shift((a+f+k[i]+M[g]), s[i]);
        a = tmp;
    }
    atemp += a;
    btemp += b;
    ctemp += c;
    dtemp += d;
}

/*
 *填充函数
 *处理后应满足bits≡448(mod512),字节就是bytes≡56（mode64)
 *填充方式为先加一个1,其它位补零
 *最后加上64位的原来长度
 */
inline std::vector<unsigned> add(std::string_view str) {
    unsigned long num = ((str.length() + 8) / 64) + 1; //以512位,64个字节为一组
    std::vector<unsigned> strByte(num * 16); //64/4=16,所以有16个整数
    for (unsigned i = 0; i < str.length(); i++) {
        // 一个整数存储四个字节，i>>2表示i/4 一个unsigned对应4个字节，保存4个字符信息
        strByte[i >> 2] |= (str[i]) << ((i % 4) * 8);
    }

    //尾部添加1 一个unsigned保存4个字符信息,所以用128左移
    strByte[str.length() >> 2] |= 0x80 << (((str.length() % 4)) * 8);
    /*
     *添加原长度，长度指位的长度，所以要乘8，然后是小端序，所以放在倒数第二个,这里长度只用了32位
     */
    strByte[num * 16 - 2] = (unsigned) str.length() * 8;
    return strByte;
}

inline std::string changeHex(int a) {
    int b;
    std::string str, str1;
    for (int i = 0; i < 4; i++) {
        str1.clear();
        b = ((a >> i * 8) % (1 << 8)) & 0xff; //逆序处理每个字节
        for (int j = 0; j < 2; j++) {
            str1.insert(0, 1, str16[b % 16]);
            b /= 16;
        }
        str += str1;
    }
    return str;
}

}

inline std::string md5(std::string_view src) {
    unsigned atemp = 0x67452301;
    unsigned btemp = 0xefcdab89;
    unsigned ctemp = 0x98badcfe;
    unsigned dtemp = 0x10325476;
    auto strByte = detail::add(src);
    for (unsigned i = 0; i < strByte.size() / 16; i++) {
        unsigned num[16];
        for (unsigned j = 0; j < 16; j++)
            num[j] = strByte[i * 16 + j];
        detail::mainLoop(num, atemp, btemp, ctemp, dtemp);
    }
    return detail::changeHex(atemp)
        +  detail::changeHex(btemp)
        +  detail::changeHex(ctemp)
        +  detail::changeHex(dtemp);
}

#undef F
#undef G
#undef H
#undef I
#undef shift