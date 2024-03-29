#ifndef SEMANA_STATIC_ERROR_H
#define SEMANA_STATIC_ERROR_H

#include <iostream>

#include "Utils/ParserValues.h"

enum class ErrorMessage {
    /// An argument that must be a ground value is possibly not a ground value.
    argument_is_not_ground,

    /// An argument that must be ground contains an anonymous variable.
    argument_is_not_ground_anonymous,

    /// The user has attempted to define something with the same name as a builtin.
    builtin_redefined,

    /// A constructor was invoked with the wrong number of arguments.
    constructor_argument_count,

    /// The "continue" keyword was used inside of an implication for a predicate.
    continue_in_predicate_impl,

    /// An effect was invoked with the wrong number of arguments.
    effect_argument_count,

    // A predicate uses another predicate in its subproof, but does not provide
    // a handler for one of the subproof's unhandled effects.
    effect_from_predicate_unhandled,

    /// There are multiple definitions for a single effect type.
    effect_redefined,

    /// An effect type reference could not be matched to its definition.
    effect_type_undefined,

    /// An effect constructor could not be resolved to any effect type in the program.
    effect_constructor_undefined,

    /// An effect is used inside of a predicate without a handler or being declared
    /// as unhandled.
    effect_unhandled,

    /// The head of an effect implication isn't a constructor of the handled effect type.
    effect_impl_head_mismatches_effect,

    /// The head of an implication isn't a reference to the enclosing predicate.
    impl_head_mismatches_predicate,

    /// A parameter marked `in` contains a variable definition.
    input_only_argument_contains_variable_definition,

    int_literal_not_convertible,
    predicate_argument_count,

    /// A predicate was defined multiple times.
    predicate_redefined,

    string_literal_not_convertible,
    type_redefined,
    undefined_predicate,
    undefined_type,
    unknown_constructor,
    unknown_constructor_or_variable,
    variable_redefined,
    variable_type_mismatch,
};

std::string formatString(ErrorMessage msg);
std::ostream& operator<<(std::ostream &out, const ErrorMessage &msg);

class ErrorEmitter {
public:
    ErrorEmitter(std::ostream &out): out(out) {}

    // It would be preferable to implement emit as a variadic function,
    // but there are several drawbacks to doing so:
    //   1. args cannot be std::string due to non-trivial destructor
    //   2. call sites require verbose c_str call
    //   3. template methods cannot be virtual, which prevents mocking
    
    virtual void emit(SourceLocation, ErrorMessage) const;
    virtual void emit(SourceLocation, ErrorMessage, std::string) const;
    virtual void emit(SourceLocation, ErrorMessage, std::string, std::string) const;
    virtual void emit(SourceLocation, ErrorMessage, std::string, std::string, std::string) const;
    
    // template <typename ... Args>
    // void emit(ErrorMessage msg, Args&& ... args) {
    //     const std::string msg_str = formatString(msg);
    //     const size_t length = snprintf(nullptr, 0, msg_str.c_str(), args...) + 1;

    //     char *buffer = (char *) malloc(length);
    //     snprintf(buffer, length, msg_str.c_str(), args...);
    //     out << buffer << "\n";
    //     free(buffer);
    // }

    unsigned int getErrors() { return errors; }

private:
    std::ostream &out;
    mutable unsigned int errors = 0;
};

#endif // SEMANA_STATIC_ERROR_H
