#ifndef SEMANA_STATIC_ERROR_H
#define SEMANA_STATIC_ERROR_H

#include <iostream>

#include "values/ParserValues.h"

enum class ErrorMessage {
    constructor_argument_count,
    impl_head_mismatches_predicate,
    predicate_argument_count,
    undefined_predicate,
    undefined_type,
    unknown_constructor,
    unknown_constructor_or_variable,
    variable_redefined,
    variable_type_mismatch,
    variable_defined_in_body,
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
