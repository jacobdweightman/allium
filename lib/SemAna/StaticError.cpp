#include <assert.h>
#include <stdio.h>

#include "SemAna/StaticError.h"

std::string formatString(ErrorMessage msg) {
    switch(msg) {
    case ErrorMessage::undefined_predicate:
        return "Use of undefined predicate \"%s\".";
    case ErrorMessage::undefined_type:
        return "Use of undefined type \"%s\".";
    case ErrorMessage::impl_head_mismatches_predicate:
        return "Head of implication must match predicate. Did you mean \"%s\"?";
    case ErrorMessage::unknown_constructor:
        return "\"%s\" is not a known constructor of type %s.";
    }
    // The preceding switch should be exhaustive. Add this to support VC++.
    printf("Error - unhandled case: %d\n", (int) msg);
    assert(false);
    return "";
}

std::ostream& operator<<(std::ostream &out, const ErrorMessage &msg) {
    return out << formatString(msg);
}

void ErrorEmitter::emit(SourceLocation loc, ErrorMessage msg) const {
    out << "error " << loc << " - " << formatString(msg) << "\n";
    ++errors;
}

void ErrorEmitter::emit(SourceLocation loc, ErrorMessage msg, std::string a1) const {
    const std::string msg_str = formatString(msg);
    const size_t length = snprintf(nullptr, 0, msg_str.c_str(), a1.c_str()) + 1;

    char *buffer = (char *) malloc(length);
    snprintf(buffer, length, msg_str.c_str(), a1.c_str());
    out << "error " << loc << " - " << buffer << "\n";
    free(buffer);
    ++errors;
}

void ErrorEmitter::emit(SourceLocation loc, ErrorMessage msg, std::string a1, std::string a2) const {
    const std::string msg_str = formatString(msg);
    const size_t length = snprintf(nullptr, 0, msg_str.c_str(),
        a1.c_str(), a2.c_str()) + 1;

    char *buffer = (char *) malloc(length);
    snprintf(buffer, length, msg_str.c_str(), a1.c_str(), a2.c_str());
    out << "error " << loc << " - " << buffer << "\n";
    free(buffer);
    ++errors;
}
