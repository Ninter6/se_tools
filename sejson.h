#pragma once

#include <cctype>
#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <variant>
#include <unordered_map>

namespace st {

#define ERROR(s) do{throw std::logic_error(s);}while(0)

class JsonElement;
using JsonObject = std::unordered_map<std::string, JsonElement>;
using JsonArray = std::vector<JsonElement>;

class JsonElement {
public:
    enum class Type {
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_STRING,
        JSON_NUMBER,
        JSON_BOOL,
        JSON_NULL
    };

    using Value_t = std::variant<JsonObject, JsonArray, std::string, double, bool>;
    
    JsonElement() = default;

    JsonElement(JsonObject value_object) {
        value(value_object);
    }
    JsonElement(JsonArray value_array) {
        value(value_array);
    }
    JsonElement(const char* value_string) {
        value(std::string{value_string});
    }
    JsonElement(const std::string& value_string) {
        value(value_string);
    }
    template <class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
    JsonElement(T value_number) {
        value(static_cast<double>(value_number));
    }
    JsonElement(bool value_bool) {
        value(value_bool);
    }
    
    void value(const JsonObject& value_object) {
        type = Type::JSON_OBJECT;
        val= value_object;
    }
    void value(const JsonArray& value_array) {
        type = Type::JSON_ARRAY;
        val = value_array;
    }
    void value(const std::string& value_string) {
        type = Type::JSON_STRING;
        val = value_string;
    }
    void value(double value_number) {
        type = Type::JSON_NUMBER;
        val = value_number;
    }
    void value(bool value_bool) {
        type = Type::JSON_BOOL;
        val = value_bool;
    }

    JsonObject& AsObject() {
        if (type != Type::JSON_OBJECT) ERROR("Type of JsonElement isn't JsonObject!");
        return std::get<JsonObject>(val);
    }
    JsonArray& AsArray() {
        if (type != Type::JSON_ARRAY) ERROR("Type of JsonElement isn't JsonArray!");
        return std::get<JsonArray>(val);
    }
    std::string& AsString() {
        if (type != Type::JSON_STRING) ERROR("Type of JsonElement isn't String!");
        return std::get<std::string>(val);
    }
    double& AsNumber() {
        if (type != Type::JSON_NUMBER) ERROR("Type of JsonElement isn't Number!");
        return std::get<double>(val);
    }
    bool& AsBool() {
        if (type != Type::JSON_BOOL) ERROR("Type of JsonElement isn't Boolean!");
        return std::get<bool>(val);
    }

    const JsonObject& AsObject() const {
        if (type != Type::JSON_OBJECT) ERROR("Type of JsonElement isn't JsonObject!");
        return std::get<JsonObject>(val);
    }
    const JsonArray& AsArray() const {
        if (type != Type::JSON_ARRAY) ERROR("Type of JsonElement isn't JsonArray!");
        return std::get<JsonArray>(val);
    }
    const std::string& AsString() const {
        if (type != Type::JSON_STRING) ERROR("Type of JsonElement isn't String!");
        return std::get<std::string>(val);
    }
    const double& AsNumber() const {
        if (type != Type::JSON_NUMBER) ERROR("Type of JsonElement isn't Number!");
        return std::get<double>(val);
    }
    const bool& AsBool() const {
        if (type != Type::JSON_BOOL) ERROR("Type of JsonElement isn't Boolean!");
        return std::get<bool>(val);
    }
    
    Type GetType() const {
        return type;
    }

    std::string Dumps() const {
      std::stringstream ss;
      switch (type) {
        case Type::JSON_OBJECT:
          ss << std::get<JsonObject>(val);
          break;
        case Type::JSON_ARRAY:
          ss << std::get<JsonArray>(val);
          break;
        case Type::JSON_STRING:
          ss << '"' << std::get<std::string>(val) << '"';
          break;
        case Type::JSON_NUMBER:
          ss << std::get<double>(val);
          break;
        case Type::JSON_BOOL:
          ss << std::boolalpha << std::get<bool>(val);
          break;
        case Type::JSON_NULL:
          ss << "null";
          break;
      }
      return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const JsonObject& object) {
      os << "{";
      for (auto iter = object.begin(); iter != object.end(); iter++) {
        os << '\"' << iter->first << '\"' << ": " << iter->second.Dumps();
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
        os << array[i].Dumps();
        if (i != array.size() - 1) {
          os << ", ";
        }
      }
      os << "]";
      return os;
    }

private:
    Type type = Type::JSON_NULL;
    Value_t val;

};

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
    
    std::string GetStringValue() {
        return value_string;
    }
    
    double GetNumberValue() {
        return value_number;
    }
    
private:
    std::string src; // json source
    size_t current = 0; // current handling pos
    size_t prev_pos = 0; // previous handling pos
    std::string value_string;
    double value_number;
    
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
    
    JsonElement Parse() {
        JsonElement element{};
        auto token_type = scanner.Scan();
        
        switch (token_type) {
            case JsonScanner::JsonTokenType::END_OF_SOURCE:
                break;
                
            case JsonScanner::JsonTokenType::BEGIN_OBJECT: {
                auto object = ParseObject();
                element.value(object);
                break;
            }
            
            case JsonScanner::JsonTokenType::BEGIN_ARRAY: {
                auto array = ParseArray();
                element.value(array);
                break;
            }
            
            case JsonScanner::JsonTokenType::VALUE_STRING: {
                auto str = scanner.GetStringValue();
                element.value(str);
                break;
            }
                
            case JsonScanner::JsonTokenType::VALUE_NUMBER: {
                auto num = scanner.GetNumberValue();
                element.value(num);
                break;
            }
                
            case JsonScanner::JsonTokenType::LITERAL_TRUE: {
                element.value(true);
                break;
            }
                
            case JsonScanner::JsonTokenType::LITERAL_FALSE: {
                element.value(false);
                break;
            }
                
            case JsonScanner::JsonTokenType::LITERAL_NULL: {
                break;
            }
            
            default:
                break;
        }
        
        return element;
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

struct Json {
    Json() : element(JsonObject{}) {}
    Json(JsonElement element) : element(element) {}
    template<class T> Json(T arg) : element(JsonElement{arg}) {}

    static Json makeObject() {return {};} // construct as an object defaultly

    template <class...Args, 
              class = std::enable_if_t<std::disjunction_v<std::is_constructible<JsonElement, Args>...>>>
    static Json makeArray(Args...args) {return JsonArray{JsonElement{args}...};}

    JsonElement& operator[](size_t index) {
        if (element.GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        return element.AsArray()[index];
    }
    const JsonElement& operator[](size_t index) const {
        if (element.GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        return element.AsArray()[index];
    }
    
    JsonElement& operator[](const std::string& key) {
        if (element.GetType() != JsonElement::Type::JSON_OBJECT) {
            ERROR("Element isn't an object!");
        }
        return element.AsObject()[key];
    }
    const JsonElement& operator[](const std::string& key) const {
        if (element.GetType() != JsonElement::Type::JSON_OBJECT) {
            ERROR("Element isn't an object!");
        }
        return element.AsObject().at(key); // if key isn't existed, it may cause an exception
    }
    
    template <class T, 
              class = std::enable_if_t<std::is_constructible_v<JsonElement, T>>>
    void PushToArray(T arg) {
        if (element.GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        element.AsArray().push_back(JsonElement{arg});
    }

    void PushToArray(const Json& j) {
        if (element.GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        element.AsArray().push_back(j.element);
    }
    
    std::string Str() const {
        return element.Dumps();
    }

    operator JsonElement&() {return element;}
    
    JsonElement element;

};

#undef ERROR

}
