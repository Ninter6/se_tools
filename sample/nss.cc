/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-10-02 13:41:01
 * @LastEditors: Ninter6
 * @LastEditTime: 2023-11-04 11:31:36
 */
#include "seargs.h"
#include "sebase64.h"
#include "secompress.h"
#include <fstream>

#define NSS_VERSION "1.1.2"

int main(int argc, const char** argv) {
    st::ArgParser args{"Ninter's base64 and compresser program"};
    args.setProgramName("nss")
        .AddArgument("Input", "input code or file path")
        .AddOption("--decode", "-d", "decode mode")
        .AddOption("--encode", "-e", "encode mode")
        .AddOption("--compress", "-p", "compress mode")
        .AddOption("--file", "-f", "indicates the input is a file name")
        .AddOption("--base58", "-E", "use base85 algorithm")
        .AddOption("--原神启动", {}, "    原神, 启动!"/*中文有bug，没办法*/)
        .AddOption("--ikun", "-K", "你是我的，我是你的谁？")
        .AddValueOption("--output", "-o", "specify output file")
        .AddSCOption("--version", "-v", "show the program version", []{
            std::cout << "nss version: " << NSS_VERSION << std::endl;
        })
        .AddHelpOption()
        .setLossArgumentsCallBack([]{
            std::cout << "try 'nss --help' to get help" << std::endl;
            std::exit(-1);
        })
        .Parse(argc, argv);
    
    std::string code;
    if (args.OptionEnabled("--file")) {
        std::ifstream fs{args.ArgumentValue("Input")};
        if (!fs.good()) {
            std::cout << "File doesn't exist: " + args.ArgumentValue("Input");
            return -1;
        }
        code = {std::istreambuf_iterator{fs}, std::istreambuf_iterator<char>{}};
    } else {
        code = args.ArgumentValue("Input");
    }

    std::ofstream ofs;
    if (args.OptionEnabled("--output")) {
        ofs.open(args.OptionValue("--output"));
    } else {
        ofs.std::ios::rdbuf(std::cout.rdbuf());
    }

    std::function encode = args.OptionEnabled("--base58") ?
        st::base58::encode : args.OptionEnabled("--原神启动") ? 
        st::原神::encode : args.OptionEnabled("--ikun") ? 
        st::ikun::encode : st::base64::encode;
    std::function decode = args.OptionEnabled("--base58") ?
        st::base58::decode : args.OptionEnabled("--原神启动") ? 
        st::原神::decode : args.OptionEnabled("--ikun") ? 
        st::ikun::decode : st::base64::decode;

    if (args.OptionEnabled("--encode")) {
        if (args.OptionEnabled("--compress")) {
            st::Compresser cmpr{{code.begin(), code.end()}};
            cmpr.compress();
            code = cmpr.str();
        }
        ofs << encode(code);
    } else {
        code = decode(code);
        if (args.OptionEnabled("--compress")) {
            st::Compresser cmpr{{code.begin(), code.end()}};
            cmpr.decompress();
            code = cmpr.str();
        }
        ofs << code;
    }

    return 0;
}
