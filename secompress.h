#include <string>
#include <vector>
#include <algorithm>

namespace st {

/**
 * 一个压缩容器, 使用了貌似是我自创的算法
 * 如果存储的是明文可以使用compress()压缩
 * 如果存储的是密文可以使用decompress()解压
 * 注意: 不会检查是否已经压缩
 */
class Compresser {
public:
    Compresser(const std::vector<uint8_t>& data) : m_data{data} {}

    std::vector<uint8_t>& data() {return m_data;}
    const std::vector<uint8_t>& data() const {return m_data;}

    size_t size() const {return m_data.size();}

    void compress() {
        uint16_t dic_size = 256, buf_size = 256;
        int64_t dic_pos = -dic_size, buf_pos = 0;
        std::vector<uint8_t> code;
        std::vector<bool> dis;

        while (buf_pos < m_data.size()) {
            int64_t ff = dic_pos;
            uint8_t fl = 3, ld = 0;

            while ((ff = search(ff, dic_pos + dic_size, buf_pos, buf_pos + fl)) <= dic_pos + dic_size - fl) {
                if (dic_pos + dic_size - ff >= fl)
                    ld = dic_pos + dic_size - ff;
                fl++;
            }

            if (ld) {
                code.push_back(ld);
                code.push_back(--fl);
                dis.push_back(true);
                dic_pos += fl;
                buf_pos += fl;
            } else {
                code.push_back(m_data[buf_pos]);
                dis.push_back(false);
                dic_pos++;
                buf_pos++;
            }
        }

        const auto& cdis = b2c(dis);
        m_data.swap(code);
        m_data.insert(m_data.end(), cdis.rbegin(), cdis.rend());
    }

    void decompress() {
        std::vector<uint8_t> data;
        size_t c = 0, d = 0;
        while (c < m_data.size() - (d+7)/8) {
            if (*(m_data.end() - 1 - d/8) & (1 << (d % 8))) {
                auto db = data.end() - m_data[c++];
                auto de = db + m_data[c++];
                data.insert(data.end(), db, de);
                d++;
            } else {
                data.push_back(m_data[c++]);
                d++;
            }
        }
        m_data.swap(data);
    }

    std::string str() const {
        std::string str(m_data.size(), 0);
        str.replace(str.begin(), str.end(), m_data.begin(), m_data.end());
        return str;
    }

    auto begin() {return m_data.begin();}
    auto begin() const {return m_data.cbegin();}
    auto end() {return m_data.end();}
    auto end() const {return m_data.cend();}

private:
    std::vector<uint8_t> m_data;

    int64_t search(int64_t sb, int64_t se, int64_t tb, int64_t te) {
        sb = std::max(sb, 0ll);
        do {
            sb = std::distance(m_data.begin(), std::find(m_data.begin() + sb, m_data.begin() + se, m_data[tb]));
        } while(!cmp(sb, tb, te - tb) && se - ++sb >= te - tb);
        return sb;
    }

    bool cmp(int64_t ap, int64_t bp, int16_t size) {
        for (int16_t i = 0; i < size; i++)
            if (m_data[ap + i] != m_data[bp + i])
                return 0;
        return 1;
    }
    
    std::vector<uint8_t> b2c(const std::vector<bool>& boolVector) {
        std::vector<uint8_t> result;
        result.reserve((boolVector.size() + 7) / 8); // 预先分配足够的空间

        uint8_t currentByte = 0, bitCount = 0;

        for (bool b : boolVector) {
            currentByte |= b << bitCount;
            if (++bitCount == 8) {
                result.push_back(currentByte);
                currentByte = bitCount = 0;
            }
        }

        // 处理剩余的位数，如果有的话
        if (bitCount) result.push_back(currentByte);

        return result;
    }
};


}