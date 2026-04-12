//
// Created by jcen on 7.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_TYPES_H
#define LLVM_KALEIDOSCOPE_TYPES_H
#include <set>

enum class TypeIdentifier {
    VOID,
    I32,
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