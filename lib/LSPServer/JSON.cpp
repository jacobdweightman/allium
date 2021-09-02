#include "LSPServer/JSON.h"

Optional<JSON> JSONObject::getAtKey(std::string key) {
    auto value = find(key);
    return (value == end()) ? Optional<JSON>() : value->second;
}

Optional<JSON> JSON::parseNull(std::istream &in) {
    std::streampos start = in.tellg();
    char buffer[4];
    in.read(buffer, 4);

    if(strncmp(buffer, "null", 4) == 0) {
        return JSON(Unit());
    } else {
        in.clear();
        in.seekg(start);
        return Optional<JSON>();
    }
}

Optional<JSON> JSON::parseBool(std::istream &in) {
    std::streampos start = in.tellg();
    char buffer[5];
    in.read(buffer, 5);

    if(strncmp(buffer, "true", 4) == 0) {
        in.unget();
        return JSON(true);
    } else if(strncmp(buffer, "false", 5) == 0) {
        return JSON(false);
    } else {
        in.clear();
        in.seekg(start);
        return Optional<JSON>();
    }
}

Optional<JSON> JSON::parseDouble(std::istream &in) {
    double d;
    in >> d;
    if(in) {
        return JSON(d);
    } else {
        in.clear();
        return Optional<JSON>();
    }
}

Optional<std::string> JSON::parseString(std::istream &in) {
    std::streampos start = in.tellg();
    std::string str;

    if(in.get() == '"') {
        while(in && in.peek() != '"') {
            str.push_back(in.get());
        }

        if(in.get() == '"') {
            return str;
        }
    }

    in.clear();
    in.unget();
    return Optional<std::string>();
}

Optional<JSON> JSON::parseArray(std::istream &in) {
    std::streampos start = in.tellg();
    JSONArray arr;

    if(in.get() == '[') {
        do {
            JSON::parse(in).map([&](JSON json) {
                arr.push_back(json);
            });
        } while(in.get() == ',');
        in.clear();
        in.unget();

        if(in.get() == ']') {
            return JSON(arr);
        }
    }

    in.clear();
    in.seekg(start);
    return Optional<JSON>();
}

Optional<JSON> JSON::parseObject(std::istream &in) {
    std::streampos start = in.tellg();
    JSONObject o;

    if(in.get() == '{') {
        do {
            JSON::parseString(in).map([&](std::string k) {

                if(in.get() != ':')
                    return;

                JSON::parse(in).map([&](JSON v) {
                    // If there is a duplicate key, the first value wins.
                    o.emplace(k, v);
                });
            });
        } while(in.get() == ',');
        in.clear();
        in.unget();

        if(in.get() == '}') {
            return JSON(o);
        }   
    }

    in.clear();
    in.seekg(start);
    return Optional<JSON>();
}

Optional<JSON> JSON::parse(std::istream &in) {
    std::string str;
    JSON json;

    if(JSON::parseString(in).unwrapInto(str)) {
        return JSON(str);
    } if(JSON::parseNull(in).unwrapInto(json) ||
        JSON::parseBool(in).unwrapInto(json) ||
        JSON::parseDouble(in).unwrapInto(json) ||
        JSON::parseString(in).unwrapInto(str) ||
        JSON::parseArray(in).unwrapInto(json) ||
        JSON::parseObject(in).unwrapInto(json)
    ) {
        return json;
    } else {
        return Optional<JSON>();
    }
}

class JSONPrinter {
public:
    JSONPrinter(std::ostream &out): out(out) {}

    void visit(const JSON &json) {
        json.switchOver(
        [&](Unit) {
            std::cout << "null\n";
        },
        [&](bool b) {
            std::cout << (b ? "true" : "false") << "\n";
        },
        [&](double d) {
            std::cout << d << "\n";
        },
        [&](std::string str) {
            std::cout << "\"" << str << "\"\n";
        },
        [&](JSONArray v) {
            if(v.empty()) {
                std::cout << "[]\n";
                return;
            }

            std::cout << "[\n";
            ++depth;
            for(const auto &x : v) {
                indent();
                visit(x);
            }
            --depth;
            indent();
            std::cout << "]\n";
        },
        [&](JSONObject o) {
            if(o.empty()) {
                std::cout << "{}\n";
                return;
            }

            std::cout << "{\n";
            ++depth;
            for(const auto &kv : o) {
                indent();
                std::cout << kv.first << ": ";
                visit(kv.second);
            }
            --depth;
            indent();
            std::cout << "}\n";
        });
    }
private:
    void indent() {
        for(int i=0; i<depth; ++i) {
            std::cout << "    ";
        }
    }
    std::ostream &out;
    int depth = 0;
};

std::ostream &operator<<(std::ostream &out, const JSON &json) {
    JSONPrinter(out).visit(json);
    return out;
}

void JSON::serialize(std::ostream &out) const {
    this->switchOver(
    [&](Unit) { out << "null"; },
    [&](bool b) { out << (b ? "true" : "false"); },
    [&](double d) { out << d; },
    [&](std::string str) { out << "\"" << str << "\""; },
    [&](JSONArray v) {
        out << "[";
        for(const auto &subJson : v) {
            if(&subJson != &(*v.begin())) {
                out << ",";
            }
            subJson.serialize(out);
        }

        out << "]";
    },
    [&](JSONObject o) {
        out << "{";
        for(const auto &pair : o) {
            if(&pair != &(*o.begin())) {
                out << ",";
            }
            out << "\"" << pair.first << "\":";
            pair.second.serialize(out);
        }
        out << "}";
    });
}
