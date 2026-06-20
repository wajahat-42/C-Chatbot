// backend/include/parser.h
#pragma once
#include "formula.h"
#include <stdexcept>
#include <string>

struct ParseError : std::runtime_error {
    explicit ParseError(const std::string& msg)
        : std::runtime_error(msg) {}
};

class Parser {
    std::string src;
    std::size_t pos = 0;

    void        skip();
    bool        peek(const std::string& s);
    bool        eat (const std::string& s);

    FormulaPtr  parseBicond();
    FormulaPtr  parseImpl();
    FormulaPtr  parseOr();
    FormulaPtr  parseAnd();
    FormulaPtr  parseUnary();
    FormulaPtr  parsePrimary();

public:
    explicit Parser(const std::string& input) : src(input) {}
    FormulaPtr parse();
};
