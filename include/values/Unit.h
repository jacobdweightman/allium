
/// Represents a type with one possible value.
///
/// Just as `void` cannot be instantiated, `Unit` can be used in places where a
/// value is expected but irrelevant.
struct Unit {
    Unit() {}

    /// This type has one possible value, which is trivially equal to itself.
    bool operator == (const Unit right) const { return true; }
};
