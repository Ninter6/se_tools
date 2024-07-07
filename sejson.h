#pragma once

#include <cassert>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace st {

#define ERROR(s) do{throw std::logic_error(s);}while(0)

#define JSON_TYPE_NON_NULL(f) \
    f(Object) \
    f(Array) \
    f(String) \
    f(Number) \
    f(Boolean)
#define JSON_TYPE(f) \
    JSON_TYPE_NON_NULL(f) \
    f(Null)

enum class JsonType {
#define g(x) x,
    JSON_TYPE(g)
#undef g
};

class Json;
struct JsonElementBase;

using JsonObject = std::unordered_map<std::string, Json>;
using JsonArray = std::vector<Json>;
using JsonString = std::string;
using JsonNumber = double;
using JsonBoolean = bool;

class Json {
public:
    Json();
    Json(const Json& o);
    Json(Json&& o) noexcept;

    Json& operator=(const Json& o);
    Json& operator=(Json&& o) noexcept;

    template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
    Json(T n);
    Json(const char*);

#define g(x) Json(const Json##x& v);
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) Json(Json##x&& v);
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) Json##x& as##x();
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) const Json##x& as##x() const;
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) bool is##x() const;
    JSON_TYPE(g);
#undef g

#define g(x) static Json Make##x() {return Json(Json##x{});}
    JSON_TYPE_NON_NULL(g)
    static Json MakeNull() {return Json{};}
#undef g

    std::string dumps() const;
    JsonType type() const;

    Json& operator[](const std::string& k) {
        assert(isObject() && "Element isn't an object!");
        return asObject()[k];
    }

    const Json& operator[](const std::string& k) const {
        assert(isObject() && "Element isn't an object!");
        return asObject().at(k);
    }

    Json& operator[](size_t n) {
        assert(isArray() && "Element isn't an array!");
        return asArray()[n];
    }

    const Json& operator[](size_t n) const {
        assert(isArray() && "Element isn't an array!");
        return asArray()[n];
    }

    friend std::ostream& operator<<(std::ostream& os, const Json& json);
    friend std::ostream& operator<<(std::ostream& os, const JsonObject& object) {
        os << "{";
        for (auto iter = object.begin(); iter != object.end(); iter++) {
            os << '\"' << iter->first << '\"' << ": " << iter->second;
            if (std::next(iter) != object.end()) {
                os << ", ";
            }
        }
        os << "}";
        return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const JsonArray& array) {
        os << "[";
        for (size_t i = 0; i < array.size(); i++) {
            os << array[i];
            if (i != array.size() - 1) {
                os << ", ";
            }
        }
        os << "]";
        return os;
    }

private:
    std::unique_ptr<JsonElementBase> element;

};

struct JsonElementBase {
    virtual ~JsonElementBase() = default;

    [[nodiscard]] virtual JsonElementBase* clone() const = 0;

#define g(x) virtual Json##x& as##x() = 0;
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) virtual const Json##x& as##x() const = 0;
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) virtual bool is##x() const = 0;
    JSON_TYPE(g);
#undef g

    [[nodiscard]] virtual std::string dumps() const = 0;
    [[nodiscard]] virtual JsonType type() const = 0;

};

namespace Impl {

struct JsonElement : public JsonElementBase {
#define g(x) virtual Json##x& as##x() {ERROR("Call 'asXXX()' with wrong type!");}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) virtual const Json##x& as##x() const {ERROR("Call 'asXXX()' with wrong type!");}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) virtual bool is##x() const {return false;}
    JSON_TYPE(g);
#undef g

};

#define BEGIN_ELEMENT(n) \
    struct JsonElement##n : public JsonElement { \
        Json##n val; \
        JsonElement##n() = default; \
        JsonElement##n(const Json##n& val) : val(val) {} \
        JsonElement##n(Json##n&& val) : val(std::move(val)) {} \
        JsonElementBase* clone() const override {return new JsonElement##n(val);} \
        Json##n& as##n() override {return val;} \
        const Json##n& as##n() const override {return val;} \
        bool is##n() const override {return true;} \
        JsonType type() const override {return JsonType:: n;}
#define END_ELEMENT() };

BEGIN_ELEMENT(Object)
    std::string dumps() const override {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }
END_ELEMENT()

BEGIN_ELEMENT(Array)
    virtual std::string dumps() const override {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }
END_ELEMENT()

BEGIN_ELEMENT(String)
    std::string dumps() const override {
        return '"' + val + '"';
    }
END_ELEMENT()

BEGIN_ELEMENT(Number)
    std::string dumps() const override {
        auto s = std::to_string(val);
        if (s.find('.') < s.size()) {
            auto n = s.find_last_not_of('0');
            s.erase(s[n-1] == '.' ? n-1 : n);
        }
        return s;
    }
END_ELEMENT()

BEGIN_ELEMENT(Boolean)
    std::string dumps() const override {
        return val ? "true" : "false";
    }
END_ELEMENT()

struct JsonElementNull : public JsonElement {
    JsonElementNull() = default;
    JsonElementBase* clone() const override {return new JsonElementNull();}
    bool isNull() const override {return true;}
    JsonType type() const override {return JsonType::Null;}
    std::string dumps() const override {return "null";}
};

#undef BEGIN_ELEMENT
#undef END_ELEMENT

}

inline Json::Json() : element(std::make_unique<Impl::JsonElementNull>()) {}
inline Json::Json(const Json& o) : element(o.element->clone()) {}
inline Json::Json(Json&& o) noexcept : element(std::move(o.element)) {}

inline Json& Json::operator=(const Json& o) {
    element.reset(o.element->clone());
    return *this;
}
inline Json& Json::operator=(Json&& o) noexcept {
    element = std::move(o.element);
    return *this;
}

template<class T, class>
inline Json::Json(T n) : element(std::make_unique<Impl::JsonElementNumber>(static_cast<double>(n))) {}
inline Json::Json(const char* s) : element(std::make_unique<Impl::JsonElementString>(s)) {}

#define g(x) inline Json::Json(const Json##x& v) : element(std::make_unique<Impl::JsonElement##x>(v)) {}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) inline Json::Json(Json##x&& v) : element(std::make_unique<Impl::JsonElement##x>(std::move(v))) {}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) inline Json##x& Json::as##x() {return element->as##x();}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) inline const Json##x& Json::as##x() const {return element->as##x();}
    JSON_TYPE_NON_NULL(g)
#undef g

#define g(x) inline bool Json::is##x() const {return element->is##x();}
    JSON_TYPE(g)
#undef g

inline std::string Json::dumps() const {
    return element->dumps();
}

inline JsonType Json::type() const {
    return element->type();
}

inline std::ostream& operator<<(std::ostream& os, const Json& json) {
    return os << json.element->dumps();
}

class JsonScanner {
public:
    JsonScanner() = default;
    JsonScanner(const std::string& source) : src(source) {}

    enum class JsonTokenType {
        BEGIN_OBJECT, // {
        END_OBJECT, // }

        VALUE_SEPARATOR, // ,
        NAME_SEPARATOR, // :

        VALUE_STRING, // "string"
        VALUE_NUMBER, // 1, 2, ...

        LITERAL_TRUE, // true
        LITERAL_FALSE, // false
        LITERAL_NULL, // null

        BEGIN_ARRAY, // [
        END_ARRAY, // ]

        END_OF_SOURCE // EOF
    };

    JsonTokenType Scan() {
        if (IsAtEnd()) {
            return JsonTokenType::END_OF_SOURCE;
        }

        prev_pos = current;

        char c = Advance();
        switch (c) {
            case '{':
                return JsonTokenType::BEGIN_OBJECT;
            case '}':
                return JsonTokenType::END_OBJECT;
            case '[':
                return JsonTokenType::BEGIN_ARRAY;
            case ']':
                return JsonTokenType::END_ARRAY;
            case ':':
                return JsonTokenType::NAME_SEPARATOR;
            case ',':
                return JsonTokenType::VALUE_SEPARATOR;
            case 't':
                ScanTrue();
                return JsonTokenType::LITERAL_TRUE;
            case 'f':
                ScanFlase();
                return JsonTokenType::LITERAL_FALSE;
            case 'n':
                ScanNull();
                return JsonTokenType::LITERAL_NULL;
            case '"':
                ScanString();
                return JsonTokenType::VALUE_STRING;

            default:
                if (std::isdigit(c) || c == '+' || c == '-') {
                    ScanNumber();
                    return JsonTokenType::VALUE_NUMBER;
                }
                if (std::isspace(c)) {
                    return Scan();
                }
                ERROR(std::string("Unsupported Token: ") + c);
                break;
        }
    }

    void Rollback() {
        current = prev_pos;
    }

    JsonString GetStringValue() {
        return value_string;
    }

    JsonNumber GetNumberValue() {
        return value_number;
    }

private:
    std::string src; // json source
    size_t current = 0; // current handling pos
    size_t prev_pos = 0; // previous handling pos
    JsonString value_string;
    JsonNumber value_number;

    bool IsAtEnd() {
        return current >= src.size();
    }

    char Advance() {
        if (current < src.size()) return src[current++];
        else return 0;
    }

    char Peek() {
        if (current < src.size()) return src[current];
        else return 0;
    }

    char PeekNext() {
        if (current < src.size()) return src[current + 1];
        else return 0;
    }

    void ScanTrue() {
        if (src.compare(current, 3, "rue") == 0) {
            current += 3;
        } else {
            ERROR("Scan 'true' error");
        }
    }

    void ScanFlase() {
        if (src.compare(current, 4, "alse") == 0) {
            current += 4;
        } else {
            ERROR("Scan 'false' error");
        }
    }

    void ScanNull() {
        if (src.compare(current, 3, "ull") == 0) {
            current += 3;
        } else {
            ERROR("Scan 'null' error");
        }
    }

    void ScanNumber() {
        auto pos = current - 1;

        bool dotflag = true, eflag = true;

        while (std::isdigit(Peek()) ||
               (Peek() == '.' && dotflag) ||
               ((Peek() == 'e' || Peek() == 'E') && eflag)) {
            if (Peek() == '.') {
                dotflag = false;
            } else if (Peek() == 'e' || Peek() == 'E') {
                eflag = false;
                Advance(); // '+' and '-' may follow 'e'
            }
            Advance();
        }

        value_number = std::stof(src.substr(pos, current - pos));
    }

    void ScanString() {
        std::string r;
        char c;
        while((c = Advance()) != '"' && !IsAtEnd()) {
            if (c == '\\') {
                switch (c = Advance()) {
                    case '\\':
                        r += '\\';
                        break;
                    case 'n':
                        r += '\n';
                        break;
                    case 't':
                        r += '\t';
                        break;
                    case 'r':
                        r += '\r';
                        break;
                    case 'b':
                        r += '\b';
                        break;
                    case 'f':
                        r += '\f';
                        break;
                    case 'a':
                        r += '\a';
                        break;
                    case 'v':
                        r += '\v';
                        break;
                    default:
                        r += c;
                        break;
                }
            } else {
                r += c;
            }
        }
        if (IsAtEnd()) {
            ERROR("Invalid string: missing closing quote!");
        }
        value_string = r;
    }
};

class JsonParser {
public:
    JsonParser() = default;
    JsonParser(const std::string& str) : scanner(str) {}
    JsonParser(const JsonScanner& scanner) : scanner(scanner) {}

    Json Parse() {
        auto token_type = scanner.Scan();

        switch (token_type) {
            case JsonScanner::JsonTokenType::BEGIN_OBJECT:
                return {ParseObject()};

            case JsonScanner::JsonTokenType::BEGIN_ARRAY:
                return {ParseArray()};

            case JsonScanner::JsonTokenType::VALUE_STRING: {
                const auto& str = scanner.GetStringValue();
                return {str};
            }

            case JsonScanner::JsonTokenType::VALUE_NUMBER: {
                auto num = scanner.GetNumberValue();
                return {num};
            }

            case JsonScanner::JsonTokenType::LITERAL_TRUE:
                return {true};

            case JsonScanner::JsonTokenType::LITERAL_FALSE:
                return {false};

            case JsonScanner::JsonTokenType::LITERAL_NULL:
            case JsonScanner::JsonTokenType::END_OF_SOURCE:
            default:
                return {};
        }
    }

private:
    JsonScanner scanner;

    JsonObject ParseObject() {
        JsonObject rst{};

        auto next = scanner.Scan();
        if (next == JsonScanner::JsonTokenType::END_OBJECT) {
            return rst;
        }
        scanner.Rollback();

        while (true) {
            next = scanner.Scan();
            if (next != JsonScanner::JsonTokenType::VALUE_STRING) {
                ERROR("Key must be string!");
            }
            std::string key = scanner.GetStringValue();
            next = scanner.Scan();
            if (next != JsonScanner::JsonTokenType::NAME_SEPARATOR) {
                ERROR("Expected ':'!");
            }
            rst.insert({key, Parse()});
            next = scanner.Scan();
            if (next == JsonScanner::JsonTokenType::END_OBJECT) {
                break;
            }
            if (next != JsonScanner::JsonTokenType::VALUE_SEPARATOR) {
                ERROR("Expected ','!");
            }
        }

        return rst;
    }

    JsonArray ParseArray() {
        JsonArray rst;

        auto next = scanner.Scan();
        if (next == JsonScanner::JsonTokenType::END_ARRAY) {
            return rst;
        }
        scanner.Rollback();

        while (true) {
            rst.push_back(Parse());
            next = scanner.Scan();

            if (next == JsonScanner::JsonTokenType::END_ARRAY) {
                break;
            }

            if (next != JsonScanner::JsonTokenType::VALUE_SEPARATOR) {
                ERROR("Expected ','!");
            }
        }

        return rst;
    }
};

#undef ERROR
#undef JSON_TYPE
#undef JSON_TYPE_NON_NULL

}