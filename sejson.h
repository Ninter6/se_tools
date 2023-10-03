#pragma once

#include <cctype>
#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <unordered_map>

namespace st {

#define ERROR(s) do{throw std::logic_error(s);}while(0)

class JsonElement;

using JsonElementPtr_t = std::shared_ptr<JsonElement>;

using JsonObject = std::unordered_map<std::string, JsonElementPtr_t>;

using JsonArray = std::vector<JsonElementPtr_t>;

using JsonObjectPtr_t = std::shared_ptr<JsonObject>;
using JsonArrayPtr_t = std::shared_ptr<JsonArray>;

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

    struct Value {
        JsonObjectPtr_t value_object;
        JsonArrayPtr_t value_array;
        std::string value_string;
        double value_number;
        bool value_bool;
    };
    
    JsonElement() = default;

    JsonElement(JsonObjectPtr_t value_object) {
        value(value_object);
    }
    JsonElement(JsonArrayPtr_t value_array) {
        value(value_array);
    }
    JsonElement(const char* value_string) {
        value(std::string{value_string});
    }
    JsonElement(const std::string& value_string) {
        value(value_string);
    }
    JsonElement(long value_number) {
        value((double)value_number);
    }
    JsonElement(double value_number) {
        value(value_number);
    }
    JsonElement(bool value_bool) {
        value(value_bool);
    }
    
    void value(JsonObjectPtr_t value_object) {
        type = Type::JSON_OBJECT;
        val.value_object = value_object;
    }
    void value(JsonArrayPtr_t value_array) {
        type = Type::JSON_ARRAY;
        val.value_array = value_array;
    }
    void value(const std::string& value_string) {
        type = Type::JSON_STRING;
        val.value_string = value_string;
    }
    void value(double value_number) {
        type = Type::JSON_NUMBER;
        val.value_number = value_number;
    }
    void value(bool value_bool) {
        type = Type::JSON_BOOL;
        val.value_bool = value_bool;
    }

    JsonObjectPtr_t AsObject() const {
        if (type != Type::JSON_OBJECT) ERROR("Type of JsonElement isn't JsonObject!");
        return val.value_object;
    }
    JsonArrayPtr_t AsArray() const {
        if (type != Type::JSON_ARRAY) ERROR("Type of JsonElement isn't JsonArray!");
        return val.value_array;
    }
    std::string AsString() const {
        if (type != Type::JSON_STRING) ERROR("Type of JsonElement isn't String!");
        return val.value_string;
    }
    double AsNumber() const {
        if (type != Type::JSON_NUMBER) ERROR("Type of JsonElement isn't Number!");
        return val.value_number;
    }
    bool AsBool() const {
        if (type != Type::JSON_BOOL) ERROR("Type of JsonElement isn't Boolean!");
        return val.value_bool;
    }
    
    Type GetType() const {
        return type;
    }

    std::string Dumps() const {
      std::stringstream ss;
      switch (type) {
        case Type::JSON_OBJECT:
          ss << *(val.value_object);
          break;
        case Type::JSON_ARRAY:
          ss << *(val.value_array);
          break;
        case Type::JSON_STRING:
          ss << '"' << val.value_string << '"';
          break;
        case Type::JSON_NUMBER:
          ss << val.value_number;
          break;
        case Type::JSON_BOOL:
          ss << std::boolalpha << val.value_bool;
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
        os << '\"' << iter->first << '\"' << ": " << iter->second->Dumps();
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
        os << array[i]->Dumps();
        if (i != array.size() - 1) {
          os << ", ";
        }
      }
      os << "]";
      return os;
    }

private:
    Type type = Type::JSON_NULL;
    Value val;

};

class Scanner {
public:
    Scanner() = default;
    Scanner(const std::string& source) : src(source) {}
    
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
        
        bool dotflag = true;
        
        while (std::isdigit(Peek()) || (Peek() == '.' && dotflag)) {
            if (Peek() == '.') {
                dotflag = false;
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

class Parser {
public:
    Parser() = default;
    Parser(const std::string& str) : scanner(str) {}
    Parser(const Scanner& scanner) : scanner(scanner) {}
    
    JsonElementPtr_t Parse() {
        auto element = std::make_shared<JsonElement>();
        auto token_type = scanner.Scan();
        
        switch (token_type) {
            case Scanner::JsonTokenType::END_OF_SOURCE:
                break;
                
            case Scanner::JsonTokenType::BEGIN_OBJECT: {
                auto object = ParseObject();
                element->value(object);
                break;
            }
            
            case Scanner::JsonTokenType::BEGIN_ARRAY: {
                auto array = ParseArray();
                element->value(array);
                break;
            }
            
            case Scanner::JsonTokenType::VALUE_STRING: {
                auto str = scanner.GetStringValue();
                element->value(str);
                break;
            }
                
            case Scanner::JsonTokenType::VALUE_NUMBER: {
                auto num = scanner.GetNumberValue();
                element->value(num);
                break;
            }
                
            case Scanner::JsonTokenType::LITERAL_TRUE: {
                element->value(true);
                break;
            }
                
            case Scanner::JsonTokenType::LITERAL_FALSE: {
                element->value(false);
                break;
            }
                
            case Scanner::JsonTokenType::LITERAL_NULL: {
                break;
            }
            
            default:
                break;
        }
        
        return element;
    }
    
private:
    Scanner scanner;
    
    JsonObjectPtr_t ParseObject() {
        auto rst = std::make_shared<JsonObject>();
        
        auto next = scanner.Scan();
        if (next == Scanner::JsonTokenType::END_OBJECT) {
            return rst;
        }
        scanner.Rollback();
        
        while (true) {
            next = scanner.Scan();
            if (next != Scanner::JsonTokenType::VALUE_STRING) {
                ERROR("Key must be string!");
            }
            std::string key = scanner.GetStringValue();
            next = scanner.Scan();
            if (next != Scanner::JsonTokenType::NAME_SEPARATOR) {
                ERROR("Expected ':'!");
            }
            rst->insert({key, Parse()});
            next = scanner.Scan();
            if (next == Scanner::JsonTokenType::END_OBJECT) {
                break;
            }
            if (next != Scanner::JsonTokenType::VALUE_SEPARATOR) {
                ERROR("Expected ','!");
            }
        }
        
        return rst;
    }
    
    JsonArrayPtr_t ParseArray() {
        auto rst = std::make_shared<JsonArray>();
        
        auto next = scanner.Scan();
        if (next == Scanner::JsonTokenType::END_ARRAY) {
            return rst;
        }
        scanner.Rollback();
        
        while (true) {
            rst->push_back(Parse());
            next = scanner.Scan();
            
            if (next == Scanner::JsonTokenType::END_ARRAY) {
                break;
            }
            
            if (next != Scanner::JsonTokenType::VALUE_SEPARATOR) {
                ERROR("Expected ','!");
            }
        }
        
        return rst;
    }
};

class Json {
public:
    Json() : element(std::make_shared<JsonElement>(std::make_shared<JsonObject>())) {}
    Json(JsonElementPtr_t element) : element(element) {}
    template<class T> Json(T arg) : element(std::make_shared<JsonElement>(arg)) {}
    
    static Json makeArray() {return std::make_shared<JsonArray>();}

    JsonElement& operator[](size_t index) const {
        if (element->GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        return *(*element->AsArray())[index];
    }
    
    JsonElement& operator[](const std::string& key) const {
        if (element->GetType() != JsonElement::Type::JSON_OBJECT) {
            ERROR("Element isn't an object!");
        }
        if (element->AsObject()->count(key)) {
            return *(*element->AsObject())[key];
        } else {
            auto e = std::make_shared<JsonElement>();
            element->AsObject()->insert({key, e});
            return *e;
        }
    }
    
    template<class T>
    void PushToArray(T arg) {
        if (element->GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        element->AsArray()->push_back(std::make_shared<JsonElement>(arg));
    }

    void PushToArray(const Json& j) {
        if (element->GetType() != JsonElement::Type::JSON_ARRAY) {
            ERROR("Element isn't an array!");
        }
        element->AsArray()->push_back(std::make_shared<JsonElement>((JsonElement&)j));
    }

    operator JsonElement&() const {
        return *element;
    }
    
    std::string Str() const {
        return element->Dumps();
    }
    
private:
    JsonElementPtr_t element;
};

}
