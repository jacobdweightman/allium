#ifndef UNIT_H
#define UNIT_H

#include <ostream>

/// Represents a type with one possible value.
///
/// Just as `void` cannot be instantiated, `Unit` can be used in places where a
/// value is expected but irrelevant.
struct Unit {
    Unit() {}

    /// This type has one possible value, which is trivially equal to itself.
    bool operator==(const Unit right) const { return true; }

    friend std::ostream &operator<<(std::ostream &out, Unit u) {
        return out << "()";
    }
};

#endif // UNIT_H
