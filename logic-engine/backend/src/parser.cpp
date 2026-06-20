// backend/src/parser.cpp
#include "parser.h"
#include <cctype>

void Parser::skip() {
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
        ++pos;
}

bool Parser::peek(const std::string& s) {
    skip();
    return src.compare(pos, s.size(), s) == 0;
}

bool Parser::eat(const std::string& s) {
    if (!peek(s)) return false;
    pos += s.size();
    return true;
}

// Grammar:
// formula := bicond
// bicond  := impl  ( '<->' impl )*     right-assoc
// impl    := or    ( '->'  impl )?     right-assoc
// or      := and   ( '|'   and  )*
// and     := unary ( '&'   unary)*
// unary   := ( '~' | '!' ) unary  |  primary
// primary := ATOM  |  '(' formula ')'

FormulaPtr Parser::parse() {
    skip();
    if (pos >= src.size())
        throw ParseError("Empty input");
    auto f = parseBicond();
    skip();
    if (pos < src.size())
        throw ParseError("Unexpected token at position " +
                         std::to_string(pos) + ": '" +
                         src.substr(pos, 8) + "'");
    return f;
}

FormulaPtr Parser::parseBicond() {
    auto left = parseImpl();
    if (eat("<->"))
        return Formula::makeBic(left, parseBicond());
    return left;
}

FormulaPtr Parser::parseImpl() {
    auto left = parseOr();
    if (eat("->"))
        return Formula::makeImpl(left, parseImpl());
    return left;
}

FormulaPtr Parser::parseOr() {
    auto left = parseAnd();
    while (eat("|") || eat("\\/"))
        left = Formula::makeOr(left, parseAnd());
    return left;
}

FormulaPtr Parser::parseAnd() {
    auto left = parseUnary();
    while (eat("&") || eat("/\\"))
        left = Formula::makeAnd(left, parseUnary());
    return left;
}

FormulaPtr Parser::parseUnary() {
    if (eat("~") || eat("!"))
        return Formula::makeNeg(parseUnary());
    return parsePrimary();
}

FormulaPtr Parser::parsePrimary() {
    skip();

    if (eat("(")) {
        auto f = parseBicond();
        if (!eat(")"))
            throw ParseError("Expected ')' at position " + std::to_string(pos));
        return f;
    }

    if (pos < src.size() &&
        (std::isalpha(static_cast<unsigned char>(src[pos])) || src[pos] == '_'))
    {
        std::string name;
        while (pos < src.size() &&
               (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_'))
            name += src[pos++];
        return Formula::makeAtom(name);
    }

    if (pos >= src.size())
        throw ParseError("Unexpected end of input");
    throw ParseError(std::string("Unexpected character '") + src[pos] +
                     "' at position " + std::to_string(pos));
}
