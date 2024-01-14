#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cassert>

namespace st {

/*
# 此库将命令行参数分为以下几类:

- param

这是所有命令行参数的基类, 也就是说所有参数必须包含两个成分
> 名称, 帮助

- option

如其名, 选项是可选的, 而且拥有一个`短名`, 可为空, 否则必须为一个`-`+`字符`的形式\
并且要求其名称为`--`+`字符串`的形式\
短名输入时允许聚合指定, 如:
```sh
ls -lh
```
> 基本的option只有**选中**和**未选**两种情况

- value_option

option的子类, 可以包含一个值, 但不确定是什么类型, 需要在获取时转换, 如:
```sh
--num 114514
```
> 如果需要用户输入指定类型, 建议在帮助中添加提示

- multivalue_option

类似value_option, 但是可以重复使用以包含多个值, 如:
```sh
ffmpeg -i file1 -i file2 -i file3
```

- short_circuit_option

option的子类, 包含一个回调函数, 在解析时如果被选中会直接调用

- argument

必须的参数, 如果某个参数没有提供，则程序会报错并退出\
其持有一个值, 输入时无需前缀
> 如果程序有多个参数, 则需要按序输入

- named_argument

命名参数, 同样是必须的参数, 但是使用时必须安照`名称=值`的格式输入, 如:
```sh
name=Alice age=16
```
> 命名参数无需按序输入
*/

struct param {
    param(const std::string& name, const std::string& help) : name(name), help(help) {}
    virtual ~param() {} // 为了多态, 没什么好清理的
    std::string name;
    std::string help;
};

struct option : public param {
    option(const std::string& name, const std::string& sname, const std::string& help)
    : param(name, help), short_name(sname) {
        assert(name.substr(0, 2) == "--" && name.size() >= 3 &&
            "Invalid name!"
        );
        assert(((short_name.front() == '-' && short_name.size() == 2) || !short_name.size()) &&
            "Invalid short name!"
        );
    }
    std::string short_name;
    bool enabled = false;
};

struct value_option : public option {
    value_option(const std::string& name, const std::string& sname, const std::string& help)
    : option(name, sname, help) {}
    std::string value;
};

struct multivalue_option : public option {
    multivalue_option(const std::string& name, const std::string& sname, const std::string& help)
    : option(name, sname, help) {}
    std::vector<std::string> value;
};

struct short_circuit_option : public option {
    short_circuit_option(const std::string& name, const std::string& sname, const std::string& help, std::function<void()> func)
    : option(name, sname, help), func(func) {}
    std::function<void()> func;
};

struct argument : public param {
    argument(const std::string& name, const std::string& help) : param(name, help) {}
    std::string value;
};

struct named_argument : public argument {
    named_argument(const std::string& name, const std::string& help) : argument(name, help) {}
};

static const std::string& s2v(const std::string& str) {return str;}

template<class T>
static T s2v(const std::string& str) {
    std::istringstream os{str};
    T t;
    os >> t;
    return t;
}

inline std::string str_replace(const std::string &str, const std::string &from, const std::string &to) {
    std::string ret;
    std::size_t pos = 0, pre_pos = 0;
    while ((pos = str.find(from, pre_pos)) != std::string::npos) {
        ret += str.substr(pre_pos, pos - pre_pos) + to;
        pre_pos = pos + from.length();
    }
    ret += str.substr(pre_pos);
    return ret;
}

class ArgParser {
public:
    ArgParser(const std::string& description) : description(description) {}

    ArgParser& setProgramName(const std::string& name) {program_name = name;return *this;}

    ArgParser& setLossArgumentsCallBack(const std::function<void()>& callback) {loss_arguments_callback = callback;return *this;}

    void PrintUsage() const {
        std::cout << "usage: " << program_name;
        for (const auto& args : arguments) std::cout << " [" << args.name << "]";
        for (const auto& args : named_arguments) std::cout << " [=" << args.name << "]";
        if (options.size()) std::cout << " [options...]" << std::endl;
    }

    void PrintHelp() const {
        PrintUsage();
        std::cout << "\n" << description << "\n\n";
        std::cout << "Options:\n";
        // calculate the longest option name
        std::size_t max_name_length = 0;
        for (const auto &opt : options) {
            std::size_t length = opt->name.length();
            if (!opt->short_name.empty()) {
                length += 4;
            }
            max_name_length = std::max(max_name_length, length);
        }
        max_name_length = std::max(max_name_length, std::size_t(25));
        // print the options
        for (const auto &opt : options) {
            std::cout << "  ";
            std::size_t printed_length = 0;
            if (!opt->short_name.empty()) {
                std::cout << opt->short_name << ", ";
                printed_length = 4;
            }
            std::cout << opt->name;
            printed_length += opt->name.length();
            std::cout << std::string(max_name_length - printed_length, ' ');
            std::cout << str_replace(opt->help, "\n", "\n" + std::string(max_name_length + 2, ' ')) << '\n';
        }
        if (named_arguments.size() > 0) {
            std::cout << "\nNamed arguments:\n";
            max_name_length = 0;
            for (const auto &arg : named_arguments) {
                max_name_length = std::max(max_name_length, arg.name.length());
            }
            max_name_length = std::max(max_name_length, std::size_t(25));
            for (const auto &arg : named_arguments) {
                std::cout << "  ";
                std::cout << arg.name;
                std::cout << std::string(max_name_length - arg.name.length(), ' ');
                std::cout << str_replace(arg.help, "\n", "\n" + std::string(max_name_length + 2, ' ')) << '\n';
            }
        }
        if (arguments.size() > 0) {
            std::cout << "\nPosition arguments:\n";
            max_name_length = 0;
            for (const auto &arg : arguments) {
                max_name_length = std::max(max_name_length, arg.name.length());
            }
            max_name_length = std::max(max_name_length, std::size_t(25));
            for (const auto &arg : arguments) {
                std::cout << "  ";
                std::cout << arg.name;
                std::cout << std::string(max_name_length - arg.name.length(), ' ');
                std::cout << str_replace(arg.help, "\n", "\n" + std::string(max_name_length + 2, ' ')) << '\n';
            }
        }

        std::exit(0); // program should close while printed help
    }

    /**
     * 这是一个可以添加任意option的模版函数
     * 如果你要添加option或value_option, 请以`名称``短名``帮助`的顺序输入
     */
    template <class T, class...Args>
    typename std::enable_if<std::is_base_of<option, T>::value || std::is_same<option, T>::value, ArgParser&>::type
    AddOption(Args...args) {
        options.push_back(std::make_unique<T>(std::forward<Args...>(args)...));
        return *this;
    }

    ArgParser& AddOption(const std::string& name, const std::string& sname, const std::string& help) {
        options.push_back(std::make_unique<option>(name, sname, help));
        return *this;
    }

    ArgParser& AddSCOption(const std::string& name, const std::string& sname, const std::string& help, std::function<void()> func) {
        options.push_back(std::make_unique<short_circuit_option>(name, sname, help, func));
        return *this;
    }

    ArgParser& AddValueOption(const std::string& name, const std::string& sname, const std::string& help) {
        options.push_back(std::make_unique<value_option>(name, sname, help));
        return *this;
    }

    ArgParser& AddHelpOption() {
        return AddSCOption("--help", "-?", "show the help message", [&]{PrintHelp();});
    }

    ArgParser& AddArgument(const std::string& name, const std::string& help) {
        arguments.emplace_back(name, help);
        return *this;
    }

    ArgParser& AddNamedArgument(const std::string& name, const std::string& help) {
        named_arguments.emplace_back(name, help);
        return *this;
    }

    void Parse(int argc, const char** args) {
        if (argc == 1) { // no arguments passed
            loss_arguments();
            return;
        }

        std::vector<std::string> tokens{args + 1, args + argc};

        std::vector<size_t> op_sn;
        std::vector<size_t> op_ln;
        std::vector<size_t> s_args;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].front() == '-') {
                if (tokens[i][1] == '-') {
                    op_ln.push_back(i);
                } else {
                    op_sn.push_back(i);
                }
            } else {
                s_args.push_back(i);
            }
        }

        for (const auto& i : op_sn) {
            for (auto c : tokens[i].substr(1)) {
                auto& op = find_option_by_sname(c);
                op->enabled = true;
                if (is_sc_option(op.get())) {
                    dynamic_cast<short_circuit_option*>(op.get())->func();
                } else if (is_value_option(op.get())) {
                    auto it = std::lower_bound(s_args.begin(), s_args.end(), i+1);
                    if(it == s_args.end() || *it != i+1) {
                        loss_arguments(op->name);
                        return;
                    }
                    s_args.erase(it);
                    dynamic_cast<value_option*>(op.get())->value = tokens[i+1];
                } else if (is_multivalue_option(op.get())) {
                    auto it = std::lower_bound(s_args.begin(), s_args.end(), i+1);
                    if(it == s_args.end() || *it != i+1) {
                        loss_arguments(op->name);
                        return;
                    }
                    s_args.erase(it);
                    dynamic_cast<multivalue_option*>(op.get())->value.push_back(tokens[i+1]);
                }
            }
        }

        for (const auto& i : op_ln) {
            auto& op = find_option(tokens[i]);
            op->enabled = true;
            if (is_sc_option(op.get())) {
                dynamic_cast<short_circuit_option*>(op.get())->func();
            } else if (is_value_option(op.get())) {
                auto it = std::lower_bound(s_args.begin(), s_args.end(), i+1);
                if(it == s_args.end() || *it != i+1) {
                    loss_arguments(op->name);
                    return;
                }
                s_args.erase(it);
                dynamic_cast<value_option*>(op.get())->value = tokens[i+1];
            } else if (is_multivalue_option(op.get())) {
                auto it = std::lower_bound(s_args.begin(), s_args.end(), i+1);
                if(it == s_args.end() || *it != i+1) {
                    loss_arguments(op->name);
                    return;
                }
                s_args.erase(it);
                dynamic_cast<multivalue_option*>(op.get())->value.push_back(tokens[i+1]);
            }
        }

        for (auto& narg : named_arguments) {
            size_t ep;
            auto it = std::find_if(s_args.begin(), s_args.end(), [&](size_t a){
                ep = tokens[a].find('=');
                return ep != std::string::npos && tokens[a].substr(0, ep) == narg.name;
            });
            if (it == s_args.end()) {
                loss_arguments(narg.name);
                return;
            }
            narg.value = tokens[*it].substr(ep + 1);
            s_args.erase(it);
        }

        if (s_args.size() != arguments.size()) {
            loss_arguments();
            return;
        }
        for (int i = 0; i < s_args.size(); i++) {
            arguments[i].value = tokens[s_args[i]];
        }
    }

    bool OptionEnabled(const std::string& name) const {
        return find_option(name)->enabled;
    }

    std::string OptionValue(const std::string& name) const {
        auto& p = find_option(name);
        assert(is_value_option(p.get()) && "Isn't option value!");
        return dynamic_cast<value_option*>(p.get())->value;
    }

    template<class T>
    T OptionValue(const std::string& name) const {
        auto& p = find_option(name);
        assert(is_value_option(p.get()) && "Isn't option value!");
        return s2v<T>(dynamic_cast<value_option*>(p.get())->value);
    }

    std::vector<std::string> OptionMultivalue(const std::string& name) const {
        auto& p = find_option(name);
        assert(is_multivalue_option(p.get()) && "Isn't multivalue option!");
        return dynamic_cast<multivalue_option*>(p.get())->value;
    }

    std::string ArgumentValue(const std::string& name) const {
        return find_argument(name).value;
    }

    template<class T>
    T ArgumentValue(const std::string& name) const {
        return s2v<T>(find_argument(name).value);
    }

    std::string NamedArgumentValue(const std::string& name) const {
        return find_named_argument(name).value;
    }

    template<class T>
    T NamedArgumentValue(const std::string& name) const {
        return s2v<T>(find_named_argument(name).value);
    }

private:
    std::string description;
    std::string program_name = "program_name"; // default program name

    std::function<void()> loss_arguments_callback = nullptr;

    std::vector<argument> arguments;
    std::vector<named_argument> named_arguments;
    std::vector<std::unique_ptr<option>> options;

private:
    void loss_arguments() const {
        PrintUsage();
        if (loss_arguments_callback) loss_arguments_callback();
    }
    void loss_arguments(const std::string& name) const {
        std::cerr << name << ": loss arguments!" << std::endl;
        if (loss_arguments_callback) loss_arguments_callback();
    }

    bool is_value_option(option* ro) const {
        return dynamic_cast<value_option*>(ro) != nullptr;
    }

    bool is_multivalue_option(option* ro) const {
        return dynamic_cast<multivalue_option*>(ro) != nullptr;
    }

    bool is_sc_option(option* ro) const {
        return dynamic_cast<short_circuit_option*>(ro) != nullptr;
    }

    std::unique_ptr<option>& find_option(const std::string& name) {
        auto it = std::find_if(options.begin(), options.end(), [&](std::unique_ptr<option>& p){
            return p->name == name;
        });
        assert(it != options.end() && "Undefined option!");
        return *it;
    }

    const std::unique_ptr<option>& find_option(const std::string& name) const {
        auto it = std::find_if(options.begin(), options.end(), [&](const std::unique_ptr<option>& p){
            return p->name == name;
        });
        assert(it != options.end() && "Undefined option!");
        return *it;
    }

    std::unique_ptr<option>& find_option_by_sname(char sn) {
        auto it = std::find_if(options.begin(), options.end(), [&](std::unique_ptr<option>& p){
            return p->short_name.back() == sn;
        });
        assert(it != options.end() && "Undefined option short name!");
        return *it;
    }

    argument& find_argument(const std::string& name) {
        auto it = std::find_if(arguments.begin(), arguments.end(), [&](argument& arg){
            return arg.name == name;
        });
        assert(it != arguments.end() && "Undefined argument!");
        return *it;
    }

    const argument& find_argument(const std::string& name) const {
        auto it = std::find_if(arguments.begin(), arguments.end(), [&](const argument& arg){
            return arg.name == name;
        });
        assert(it != arguments.end() && "Undefined argument!");
        return *it;
    }

    named_argument& find_named_argument(const std::string& name) {
        auto it = std::find_if(named_arguments.begin(), named_arguments.end(), [&](named_argument& arg){
            return arg.name == name;
        });
        assert(it != named_arguments.end() && "Undefined argument!");
        return *it;
    }

    const named_argument& find_named_argument(const std::string& name) const {
        auto it = std::find_if(named_arguments.begin(), named_arguments.end(), [&](const named_argument& arg){
            return arg.name == name;
        });
        assert(it != named_arguments.end() && "Undefined argument!");
        return *it;
    }
};

}