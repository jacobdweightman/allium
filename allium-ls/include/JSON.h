#ifndef JSON_H
#define JSON_H

#include <map>
#include <sstream>
#include <vector>

#include "Utils/TaggedUnion.h"
#include "Utils/Unit.h"

class JSON;

typedef std::vector<JSON> JSONArray;

class JSONObject : public std::map<std::string, JSON> {
public:
    using std::map<std::string, JSON>::map;

    Optional<JSON> getAtKey(std::string key);
};

typedef TaggedUnion<
    Unit, // JSON null
    bool,
    double,
    std::string,
    JSONArray,
    JSONObject
> JSONBase;

class JSON : public JSONBase {
    static Optional<JSON> parseNull(std::istream &in);
    static Optional<JSON> parseBool(std::istream &in);
    static Optional<JSON> parseDouble(std::istream &in);
    static Optional<std::string> parseString(std::istream &in);
    static Optional<JSON> parseArray(std::istream &in);
    static Optional<JSON> parseObject(std::istream &in);

public:
    using JSONBase::JSONBase;

    JSON(): JSONBase(Unit()) {}

    /// Parses a JSON object from the given input stream.
    static Optional<JSON> parse(std::istream &in);

    /// Writes a whitespace-free, serial representation of the JSON onto the
    /// given stream.
    void serialize(std::ostream &out) const;
};

std::ostream &operator<<(std::ostream &out, const JSON &json);

#endif // JSON_H
