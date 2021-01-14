#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

/// A line and column within a source file.
struct SourceLocation {
    int lineNumber;
    int columnNumber;

    SourceLocation(): lineNumber(-1), columnNumber(-1) {}

    SourceLocation(int lineNumber, int columnNumber):
        lineNumber(lineNumber), columnNumber(columnNumber) {}

    friend bool operator==(const SourceLocation &lhs, const SourceLocation &rhs) {
        return lhs.lineNumber == rhs.lineNumber && lhs.columnNumber == rhs.columnNumber;
    }

    friend bool operator!=(const SourceLocation &lhs, const SourceLocation &rhs) {
        return !(lhs == rhs);
    }
    
    friend std::ostream& operator<<(std::ostream &out, SourceLocation loc) {
        return out << loc.lineNumber << ":" << loc.columnNumber;
    }
};

/// Represents the name of an AST node. Just a strongly typed std::string.
template <typename Node>
struct Name {
    Name() {}
    explicit Name(std::string wrapped): wrapped(wrapped) {}
    Name(const char *literal): wrapped(std::string(literal)) {}

    friend inline bool operator==(const Name<Node> &left, const Name<Node> &right) {
        return left.wrapped == right.wrapped;
    }

    friend inline bool operator!=(const Name<Node> &left, const Name<Node> &right) {
        return left.wrapped != right.wrapped;
    }

    friend inline bool operator<(const Name<Node> &left, const Name<Node> &right) {
        return left.wrapped < right.wrapped;
    }

    friend inline std::ostream& operator<<(std::ostream &out, const Name<Node> &pn) {
        return out << pn.wrapped;
    }

    /// Expose a const reference to the string as a last resort for where a string
    /// is really needed.
    inline const std::string &string() const {
        return wrapped;
    }
private:
    std::string wrapped;
};

#endif // SOURCE_LOCATION_H
