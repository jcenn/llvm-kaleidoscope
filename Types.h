//
// Created by jcen on 7.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_TYPES_H
#define LLVM_KALEIDOSCOPE_TYPES_H
#include <map>
#include <set>
#include <string>

enum class TypeIdentifier {
    VOID,
    I32,
    Double,
    String,
};

inline const std::map<std::string, TypeIdentifier> type_identifiers = {
    {"void", TypeIdentifier::VOID},
    {"i32", TypeIdentifier::I32},
    {"double", TypeIdentifier::Double},
    {"string", TypeIdentifier::String},
};

enum class BinaryOperator {
    Add,
    Subtract,
    Multiply,
    CompareEQ,
    // Divide,
    // Modulus,
    // Equals,
    // NotEquals,
    // LessThan,
    // GreaterThan,
    // LessThanOrEquals,
    // GreaterThanOrEquals,
};

#endif //LLVM_KALEIDOSCOPE_TYPES_H