// backend/src/formula.cpp
#include "formula.h"

FormulaPtr Formula::makeAtom(const std::string& name) {
    auto f = std::make_shared<Formula>();
    f->type = FormulaType::ATOM;
    f->atom = name;
    return f;
}

FormulaPtr Formula::makeNeg(FormulaPtr op) {
    auto f = std::make_shared<Formula>();
    f->type = FormulaType::NEGATION;
    f->left = op;
    return f;
}

FormulaPtr Formula::makeAnd(FormulaPtr l, FormulaPtr r) {
    auto f = std::make_shared<Formula>();
    f->type  = FormulaType::CONJUNCTION;
    f->left  = l; f->right = r;
    return f;
}

FormulaPtr Formula::makeOr(FormulaPtr l, FormulaPtr r) {
    auto f = std::make_shared<Formula>();
    f->type  = FormulaType::DISJUNCTION;
    f->left  = l; f->right = r;
    return f;
}

FormulaPtr Formula::makeImpl(FormulaPtr l, FormulaPtr r) {
    auto f = std::make_shared<Formula>();
    f->type  = FormulaType::IMPLICATION;
    f->left  = l; f->right = r;
    return f;
}

FormulaPtr Formula::makeBic(FormulaPtr l, FormulaPtr r) {
    auto f = std::make_shared<Formula>();
    f->type  = FormulaType::BICONDITIONAL;
    f->left  = l; f->right = r;
    return f;
}

FormulaPtr Formula::makeBinary(FormulaType t, FormulaPtr l, FormulaPtr r) {
    switch (t) {
        case FormulaType::CONJUNCTION:   return makeAnd (l, r);
        case FormulaType::DISJUNCTION:   return makeOr  (l, r);
        case FormulaType::IMPLICATION:   return makeImpl(l, r);
        case FormulaType::BICONDITIONAL: return makeBic (l, r);
        default: throw std::logic_error("makeBinary: not a binary type");
    }
}

bool Formula::equals(const FormulaPtr& o) const {
    if (!o) return false;
    if (type != o->type) return false;
    switch (type) {
        case FormulaType::ATOM:      return atom == o->atom;
        case FormulaType::NEGATION:  return left->equals(o->left);
        default:                     return left->equals(o->left)
                                         && right->equals(o->right);
    }
}

std::string Formula::str() const {
    switch (type) {
        case FormulaType::ATOM:          return atom;
        case FormulaType::NEGATION:      return "~" + left->str();
        case FormulaType::CONJUNCTION:   return "(" + left->str() + " & "   + right->str() + ")";
        case FormulaType::DISJUNCTION:   return "(" + left->str() + " | "   + right->str() + ")";
        case FormulaType::IMPLICATION:   return "(" + left->str() + " -> "  + right->str() + ")";
        case FormulaType::BICONDITIONAL: return "(" + left->str() + " <-> " + right->str() + ")";
    }
    return "?";
}
