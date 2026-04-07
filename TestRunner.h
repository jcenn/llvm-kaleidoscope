//
// Created by jcen on 7.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_TESTRUNNER_H
#define LLVM_KALEIDOSCOPE_TESTRUNNER_H
#include <functional>
#include <map>
#include <ostream>
#include <string>

struct Token;

class LexerTestRunner {
    //std::map<> tests{};
    std::vector<std::pair<std::string, std::function<bool()>>> tests{};
    bool compare_tokens(std::vector<Token>const & expected, std::vector<Token>const & given);
public:
    LexerTestRunner();
    void runLexerTestCases();
};
#endif //LLVM_KALEIDOSCOPE_TESTRUNNER_H