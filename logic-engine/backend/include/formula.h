// backend/include/formula.h
#pragma once
#include <memory>
#include <string>
#include <stdexcept>

enum class FormulaType {
    ATOM,
    NEGATION,
    CONJUNCTION,
    DISJUNCTION,
    IMPLICATION,
    BICONDITIONAL
};

struct Formula;
using FormulaPtr = std::shared_ptr<Formula>;

struct Formula {
    FormulaType type;
    std::string  atom;
    FormulaPtr   left;
    FormulaPtr   right;

    static FormulaPtr makeAtom(const std::string& name);
    static FormulaPtr makeNeg (FormulaPtr op);
    static FormulaPtr makeAnd (FormulaPtr l, FormulaPtr r);
    static FormulaPtr makeOr  (FormulaPtr l, FormulaPtr r);
    static FormulaPtr makeImpl(FormulaPtr l, FormulaPtr r);
    static FormulaPtr makeBic (FormulaPtr l, FormulaPtr r);
    static FormulaPtr makeBinary(FormulaType t, FormulaPtr l, FormulaPtr r);

    bool equals(const FormulaPtr& other) const;
    std::string str() const;
};
