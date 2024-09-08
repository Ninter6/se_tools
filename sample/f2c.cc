#include "../seargs.h"
#include "../selog.h"
#include "../seformat.h"

#include <fstream>
#include <cstring>
#include <cctype>

using namespace std;

namespace f2c {

struct FileName {
    string filename;

    string get() const { return regex_replace(filename, regex{"\\."}, "_"); }
    string no_suffix() const {return filename.substr(0, filename.find('.'));}
};

struct FileSize {
    size_t size;
};

struct File {
    File(const string& filename) {
        ifstream ifs{filename};
        if (!ifs.good())
            throw runtime_error("failed to open file: " + filename);
        data = {istreambuf_iterator<char>{ifs}, istreambuf_iterator<char>{}};

        name.filename = filename.substr(filename.find_last_of("/\\")+1);
        size.size = data.size();
    }
    string data;
    FileName name;
    FileSize size;
};

}

template <>
struct st::Formatter<f2c::File> {
    Formatter(string_view fmt) {
        auto t = fmt.substr(fmt.find('u')+1, 1);
        switch (t.front()) {
            case '8': tp = u8;  break;
            case '3': tp = u32; break;
            case '6': tp = u64; break;
        }
        sf = string(fmt.substr(fmt.find('^')+1));
    }
    std::string operator()(const f2c::File& t){
        std::string res;
        switch (tp) {
        case u8: {
            for (int i = 0; i < t.data.size(); i++) {
                res += to_string((unsigned)t.data[i]);
                if (i != t.data.size() - 1) res += sf;
            }
            break;
        }
        case u32: {
            auto data = (uint32_t*)t.data.data();
            auto item = t.data.size()/4;
            for (int i = 0; i < item; i++) {
                res += to_string(data[i]);
                if (i != item - 1) res += sf;
            }
            break;
        }
        case u64: {
            auto data = (uint64_t*)t.data.data();
            auto item = t.data.size()/8;
            for (int i = 0; i < item; i++) {
                res += to_string(data[i]);
                if (i != item - 1) res += sf;
            }
            if (t.data.size() % 8 != 0) {
                st::log::log_warning("File does not meet the alignment requirements: {}", t.name.filename);
                uint64_t u;
                memcpy(&u, data + item, 4);
                res += sf + to_string(u);
            }
            break;
        }}
        return res;
    }
    
    enum type { u8, u32, u64 };
    type tp;
    string sf;
};

template <>
struct st::Formatter<f2c::FileName> {
    // s: no suffix, l: lower, u: upper
    Formatter(string_view fmt) {
        s = fmt.find('s') != fmt.npos;
        l = fmt.find('l') != fmt.npos;
        u = fmt.find('u') != fmt.npos;
        if (l && u)
            st::log::log_error("what do u want?");
    }
    std::string operator()(const f2c::FileName& t) {
        auto res = s ? t.no_suffix() : t.get();
        if (l) std::transform(res.begin(), res.end(), res.begin(), [](auto&&c){return tolower(c);});
        if (u) std::transform(res.begin(), res.end(), res.begin(), [](auto&&c){return toupper(c);});
        return res;
    }
    
    bool s = false, l = false, u = false;
};

template <>
struct st::Formatter<f2c::FileSize> {
    Formatter(string_view fmt) {
        if (fmt.find("u32") != fmt.npos) d = 4;
        else if (fmt.find("u64") != fmt.npos) d = 8;
    }
    std::string operator()(const f2c::FileSize& t) {
        return to_string(t.size/d + (t.size % d ? 1 : 0));
    }
    int d = 1;
};

int main (int argc, const char** argv) {
    st::ArgParser args{"File to Code."};
    args.setProgramName("f2c")
        .AddMultivalueOption("--input", "-i", "input files to convert")
        .AddArgument("output", "specify output file")
        .AddValueOption("--format", "-f", "format string or format file")
        .AddHelpOption()
        .setLossArgumentsCallBack([]{
            cout << "try 'f2c --help' to get help\n";
            exit(-1);
        })
        .setUnkonwOptionCallBack([](const auto& n) {
            cout << "unknow option: " << n << '\n'
                 << "try 'f2c --help' to get help\n";
            exit(-1);
        })
        .Parse(argc, argv);

    string fmt = "constexpr unsigned {$N:l}[{$L:u32}] = {{$D:u32^u, }};";
    if (args.OptionEnabled("--format")) {
        ifstream fmt_ifs{args.OptionValue("--format")};
        if (fmt_ifs.good())
            fmt = {istreambuf_iterator<char>{fmt_ifs}, istreambuf_iterator<char>{}};
        else
            fmt = args.OptionValue("--format");
    }
    fmt = regex_replace(fmt, regex{"\\$D"}, "0");
    fmt = regex_replace(fmt, regex{"\\$N"}, "1");
    fmt = regex_replace(fmt, regex{"\\$L"}, "2");

    ofstream op{args.ArgumentValue("output")};
    if (!op.good()) {
        st::log::log_error("Failed to open output file!");
        exit(-1);
    }

    auto ip = args.OptionMultivalue("--input");
    if (ip.empty()) {
        st::log::log_error("No input file!");
        exit(-1);
    }
    for (auto&& i : ip) {
        f2c::File file{i};
        op << st::format(fmt, file, file.name, file.size) << endl;
    }
}
