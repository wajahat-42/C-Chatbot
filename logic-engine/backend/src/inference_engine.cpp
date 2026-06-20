// backend/src/inference_engine.cpp
#include "inference_engine.h"
#include "parser.h"
#include <algorithm>
#include <functional>

using FP = FormulaPtr;
using FT = FormulaType;

static FP Neg (FP a)            { return Formula::makeNeg(a);       }
static FP And (FP a, FP b)      { return Formula::makeAnd(a, b);    }
static FP Or  (FP a, FP b)      { return Formula::makeOr(a, b);     }
static FP Impl(FP a, FP b)      { return Formula::makeImpl(a, b);   }
static FP Bic (FP a, FP b)      { return Formula::makeBic(a, b);    }
static FP Bin (FT t, FP a, FP b){ return Formula::makeBinary(t,a,b);}

// ─────────────────────────────────────────────────────────────────────────────
// KB helpers
// ─────────────────────────────────────────────────────────────────────────────

bool InferenceEngine::isKnown(const FP& f) const {
    for (const auto& s : kb_)
        if (s.formula->equals(f)) return true;
    return false;
}

bool InferenceEngine::addToKB(FP f, const std::string& rule,
                               const std::vector<int>& deps)
{
    if (!f || isKnown(f)) return false;
    ProofStep step;
    step.stepNumber = static_cast<int>(kb_.size()) + 1;
    step.formula    = f;
    step.formulaStr = f->str();
    step.ruleName   = rule;
    step.deps       = deps;
    kb_.push_back(std::move(step));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// transformRecursive
// ─────────────────────────────────────────────────────────────────────────────

std::vector<FP> InferenceEngine::transformRecursive(
    FP f,
    const std::function<FP(FP)>& transform)
{
    std::vector<FP> results;

    if (FP top = transform(f))
        results.push_back(top);

    if (f->type == FT::NEGATION) {
        for (auto& sub : transformRecursive(f->left, transform))
            results.push_back(Neg(sub));
    } else if (f->type != FT::ATOM) {
        for (auto& sub : transformRecursive(f->left, transform))
            results.push_back(Bin(f->type, sub, f->right));
        for (auto& sub : transformRecursive(f->right, transform))
            results.push_back(Bin(f->type, f->left, sub));
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// applyReplacement
// ─────────────────────────────────────────────────────────────────────────────

bool InferenceEngine::applyReplacement(
    const std::string& name,
    const std::function<FP(FP)>& transform)
{
    bool added = false;
    int  n     = static_cast<int>(kb_.size());

    for (int i = 0; i < n; ++i) {
        for (auto& derived : transformRecursive(kb_[i].formula, transform))
            if (addToKB(derived, name, {kb_[i].stepNumber}))
                added = true;
    }
    return added;
}

// ─────────────────────────────────────────────────────────────────────────────
// collectByType
// ─────────────────────────────────────────────────────────────────────────────

void InferenceEngine::collectByType(FP f, FT t,
                                    std::vector<FP>& out) const
{
    if (!f) return;
    if (f->type == t) {
        bool found = false;
        for (auto& x : out) if (x->equals(f)) { found = true; break; }
        if (!found) out.push_back(f);
    }
    if (f->type != FT::ATOM) {
        collectByType(f->left,  t, out);
        if (f->type != FT::NEGATION)
            collectByType(f->right, t, out);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RULES OF INFERENCE (9)
// ═════════════════════════════════════════════════════════════════════════════

// 1. Modus Ponens (MP): { P→Q, P } ⊢ Q
bool InferenceEngine::applyMP() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::IMPLICATION) continue;
        FP ant = kb_[i].formula->left;
        FP con = kb_[i].formula->right;
        for (int j = 0; j < n; ++j)
            if (kb_[j].formula->equals(ant))
                if (addToKB(con, "MP", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
    }
    return added;
}

// 2. Modus Tollens (MT): { P→Q, ~Q } ⊢ ~P
bool InferenceEngine::applyMT() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::IMPLICATION) continue;
        FP ant    = kb_[i].formula->left;
        FP negCon = Neg(kb_[i].formula->right);
        for (int j = 0; j < n; ++j)
            if (kb_[j].formula->equals(negCon))
                if (addToKB(Neg(ant), "MT", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
    }
    return added;
}

// 3. Hypothetical Syllogism (HS): { P→Q, Q→R } ⊢ P→R
bool InferenceEngine::applyHS() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::IMPLICATION) continue;
        FP p = kb_[i].formula->left;
        FP q = kb_[i].formula->right;
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            if (kb_[j].formula->type != FT::IMPLICATION) continue;
            if (kb_[j].formula->left->equals(q)) {
                FP r = kb_[j].formula->right;
                if (addToKB(Impl(p, r), "HS", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
            }
        }
    }
    return added;
}

// 4. Disjunctive Syllogism (DS): { P|Q, ~P } ⊢ Q
bool InferenceEngine::applyDS() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::DISJUNCTION) continue;
        FP p = kb_[i].formula->left;
        FP q = kb_[i].formula->right;
        FP negP = Neg(p), negQ = Neg(q);
        for (int j = 0; j < n; ++j) {
            if (kb_[j].formula->equals(negP))
                if (addToKB(q, "DS", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
            if (kb_[j].formula->equals(negQ))
                if (addToKB(p, "DS", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
        }
    }
    return added;
}

// 5. Constructive Dilemma (CD): { (P→Q)&(R→S), P|R } ⊢ Q|S
bool InferenceEngine::applyCD() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::CONJUNCTION) continue;
        FP lhs = kb_[i].formula->left;
        FP rhs = kb_[i].formula->right;
        if (lhs->type != FT::IMPLICATION || rhs->type != FT::IMPLICATION) continue;
        FP p = lhs->left, q = lhs->right;
        FP r = rhs->left, s = rhs->right;
        FP pOrR = Or(p, r);
        for (int j = 0; j < n; ++j)
            if (kb_[j].formula->equals(pOrR))
                if (addToKB(Or(q, s), "CD", {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
    }
    return added;
}

// 6. Destructive Dilemma (DD): { (P→Q)&(R→S), ~Q|~S } ⊢ ~P|~R
bool InferenceEngine::applyDD() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::CONJUNCTION) continue;
        FP lhs = kb_[i].formula->left;
        FP rhs = kb_[i].formula->right;
        if (lhs->type != FT::IMPLICATION || rhs->type != FT::IMPLICATION) continue;
        FP p = lhs->left, q = lhs->right;
        FP r = rhs->left, s = rhs->right;
        FP negQorNegS = Or(Neg(q), Neg(s));
        for (int j = 0; j < n; ++j)
            if (kb_[j].formula->equals(negQorNegS))
                if (addToKB(Or(Neg(p), Neg(r)), "DD",
                            {kb_[i].stepNumber, kb_[j].stepNumber}))
                    added = true;
    }
    return added;
}

// 7. Simplification (Simp): { P&Q } ⊢ P and { P&Q } ⊢ Q
bool InferenceEngine::applySimp() {
    bool added = false;
    int  n     = static_cast<int>(kb_.size());
    for (int i = 0; i < n; ++i) {
        if (kb_[i].formula->type != FT::CONJUNCTION) continue;
        if (addToKB(kb_[i].formula->left,  "Simp", {kb_[i].stepNumber})) added = true;
        if (addToKB(kb_[i].formula->right, "Simp", {kb_[i].stepNumber})) added = true;
    }
    return added;
}

// 8. Conjunction (Conj): { P, Q } ⊢ P&Q  [goal-directed]
bool InferenceEngine::applyConj(const FP& goal) {
    bool added = false;
    std::vector<FP> targets;
    collectByType(goal, FT::CONJUNCTION, targets);
    for (const auto& s : kb_)
        collectByType(s.formula, FT::CONJUNCTION, targets);

    int n = static_cast<int>(kb_.size());
    for (const auto& cg : targets) {
        FP p = cg->left, q = cg->right;
        int pIdx = -1, qIdx = -1;
        for (int i = 0; i < n; ++i) {
            if (kb_[i].formula->equals(p) && pIdx < 0) pIdx = kb_[i].stepNumber;
            if (kb_[i].formula->equals(q) && qIdx < 0) qIdx = kb_[i].stepNumber;
        }
        if (pIdx > 0 && qIdx > 0)
            if (addToKB(cg, "Conj", {pIdx, qIdx})) added = true;
    }
    return added;
}

// 9. Addition (Add): { P } ⊢ P|Q  [goal-directed]
bool InferenceEngine::applyAdd(const FP& goal) {
    bool added = false;
    std::vector<FP> targets;
    collectByType(goal, FT::DISJUNCTION, targets);
    for (const auto& s : kb_)
        collectByType(s.formula, FT::DISJUNCTION, targets);

    int n = static_cast<int>(kb_.size());
    for (const auto& dg : targets) {
        FP p = dg->left, q = dg->right;
        for (int i = 0; i < n; ++i) {
            if (kb_[i].formula->equals(p))
                if (addToKB(dg, "Add", {kb_[i].stepNumber})) added = true;
            if (kb_[i].formula->equals(q))
                if (addToKB(dg, "Add", {kb_[i].stepNumber})) added = true;
        }
    }
    return added;
}

// ═════════════════════════════════════════════════════════════════════════════
// RULES OF REPLACEMENT (10)
// ═════════════════════════════════════════════════════════════════════════════

// 10. De Morgan's (DM): ~(P&Q) ≡ ~P|~Q  and  ~(P|Q) ≡ ~P&~Q
bool InferenceEngine::applyDM() {
    return applyReplacement("DM", [](FP f) -> FP {
        if (f->type == FT::NEGATION && f->left->type == FT::CONJUNCTION)
            return Or(Neg(f->left->left), Neg(f->left->right));
        if (f->type == FT::NEGATION && f->left->type == FT::DISJUNCTION)
            return And(Neg(f->left->left), Neg(f->left->right));
        if (f->type == FT::DISJUNCTION &&
            f->left->type == FT::NEGATION && f->right->type == FT::NEGATION)
            return Neg(And(f->left->left, f->right->left));
        if (f->type == FT::CONJUNCTION &&
            f->left->type == FT::NEGATION && f->right->type == FT::NEGATION)
            return Neg(Or(f->left->left, f->right->left));
        return nullptr;
    });
}

// 11. Commutation (Com): P|Q ≡ Q|P  and  P&Q ≡ Q&P
bool InferenceEngine::applyCom() {
    return applyReplacement("Com", [](FP f) -> FP {
        if (f->type == FT::DISJUNCTION)  return Or (f->right, f->left);
        if (f->type == FT::CONJUNCTION)  return And(f->right, f->left);
        return nullptr;
    });
}

// 12. Association (Assoc): (P|Q)|R ≡ P|(Q|R)  and  (P&Q)&R ≡ P&(Q&R)
bool InferenceEngine::applyAssoc() {
    return applyReplacement("Assoc", [](FP f) -> FP {
        if (f->type == FT::DISJUNCTION && f->left->type == FT::DISJUNCTION)
            return Or(f->left->left, Or(f->left->right, f->right));
        if (f->type == FT::DISJUNCTION && f->right->type == FT::DISJUNCTION)
            return Or(Or(f->left, f->right->left), f->right->right);
        if (f->type == FT::CONJUNCTION && f->left->type == FT::CONJUNCTION)
            return And(f->left->left, And(f->left->right, f->right));
        if (f->type == FT::CONJUNCTION && f->right->type == FT::CONJUNCTION)
            return And(And(f->left, f->right->left), f->right->right);
        return nullptr;
    });
}

// 13. Distribution (Dist): P&(Q|R) ≡ (P&Q)|(P&R)  and  P|(Q&R) ≡ (P|Q)&(P|R)
bool InferenceEngine::applyDist() {
    return applyReplacement("Dist", [](FP f) -> FP {
        if (f->type == FT::CONJUNCTION && f->right->type == FT::DISJUNCTION)
            return Or(And(f->left, f->right->left), And(f->left, f->right->right));
        if (f->type == FT::CONJUNCTION && f->left->type == FT::DISJUNCTION)
            return Or(And(f->left->left, f->right), And(f->left->right, f->right));
        if (f->type == FT::DISJUNCTION && f->right->type == FT::CONJUNCTION)
            return And(Or(f->left, f->right->left), Or(f->left, f->right->right));
        if (f->type == FT::DISJUNCTION && f->left->type == FT::CONJUNCTION)
            return And(Or(f->left->left, f->right), Or(f->left->right, f->right));
        if (f->type == FT::DISJUNCTION &&
            f->left->type  == FT::CONJUNCTION &&
            f->right->type == FT::CONJUNCTION &&
            f->left->left->equals(f->right->left))
            return And(f->left->left, Or(f->left->right, f->right->right));
        if (f->type == FT::CONJUNCTION &&
            f->left->type  == FT::DISJUNCTION &&
            f->right->type == FT::DISJUNCTION &&
            f->left->left->equals(f->right->left))
            return Or(f->left->left, And(f->left->right, f->right->right));
        return nullptr;
    });
}

// 14. Double Negation (DN): ~~P ≡ P
bool InferenceEngine::applyDN() {
    return applyReplacement("DN", [](FP f) -> FP {
        if (f->type == FT::NEGATION && f->left->type == FT::NEGATION)
            return f->left->left;
        return nullptr;
    });
}

// 15. Transposition (Trans): P→Q ≡ ~Q→~P
bool InferenceEngine::applyTrans() {
    return applyReplacement("Trans", [](FP f) -> FP {
        if (f->type == FT::IMPLICATION)
            return Impl(Neg(f->right), Neg(f->left));
        return nullptr;
    });
}

// 16. Material Implication (Impl): P→Q ≡ ~P|Q
bool InferenceEngine::applyMImpl() {
    return applyReplacement("Impl", [](FP f) -> FP {
        if (f->type == FT::IMPLICATION)
            return Or(Neg(f->left), f->right);
        if (f->type == FT::DISJUNCTION && f->left->type == FT::NEGATION)
            return Impl(f->left->left, f->right);
        return nullptr;
    });
}

// 17. Material Equivalence (Equiv): P↔Q ≡ (P→Q)&(Q→P)
bool InferenceEngine::applyMEquiv() {
    return applyReplacement("Equiv", [](FP f) -> FP {
        if (f->type == FT::BICONDITIONAL) {
            FP p = f->left, q = f->right;
            return And(Impl(p, q), Impl(q, p));
        }
        if (f->type == FT::CONJUNCTION &&
            f->left->type  == FT::IMPLICATION &&
            f->right->type == FT::IMPLICATION &&
            f->left->left->equals(f->right->right) &&
            f->left->right->equals(f->right->left))
            return Bic(f->left->left, f->left->right);
        if (f->type == FT::DISJUNCTION &&
            f->left->type  == FT::CONJUNCTION &&
            f->right->type == FT::CONJUNCTION &&
            f->right->left->type  == FT::NEGATION &&
            f->right->right->type == FT::NEGATION &&
            f->right->left->left->equals(f->left->left) &&
            f->right->right->left->equals(f->left->right))
            return Bic(f->left->left, f->left->right);
        return nullptr;
    });
}

// 18. Exportation (Exp): (P&Q)→R ≡ P→(Q→R)
bool InferenceEngine::applyExp() {
    return applyReplacement("Exp", [](FP f) -> FP {
        if (f->type == FT::IMPLICATION && f->left->type == FT::CONJUNCTION)
            return Impl(f->left->left, Impl(f->left->right, f->right));
        if (f->type == FT::IMPLICATION && f->right->type == FT::IMPLICATION)
            return Impl(And(f->left, f->right->left), f->right->right);
        return nullptr;
    });
}

// 19. Tautology (Taut): P|P ≡ P  and  P&P ≡ P
bool InferenceEngine::applyTaut() {
    return applyReplacement("Taut", [](FP f) -> FP {
        if (f->type == FT::DISJUNCTION && f->left->equals(f->right))
            return f->left;
        if (f->type == FT::CONJUNCTION  && f->left->equals(f->right))
            return f->left;
        return nullptr;
    });
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN PROVE FUNCTION
// ═════════════════════════════════════════════════════════════════════════════

InferenceEngine::Result
InferenceEngine::prove(const std::vector<std::string>& premiseStrs,
                       const std::string& conclusionStr,
                       int maxIterations)
{
    kb_.clear();

    for (const auto& ps : premiseStrs) {
        if (ps.empty()) continue;
        try {
            addToKB(Parser(ps).parse(), "Premise", {});
        } catch (const ParseError& e) {
            return {false, {}, "",
                    "Parse error in premise '" + ps + "': " + e.what()};
        }
    }

    FP conclusion;
    try {
        conclusion = Parser(conclusionStr).parse();
    } catch (const ParseError& e) {
        return {false, {}, "",
                "Parse error in conclusion '" + conclusionStr + "': " + e.what()};
    }

    if (isKnown(conclusion))
        return {true, kb_,
                "Conclusion is identical to one of the premises.", ""};

    for (int iter = 0; iter < maxIterations; ++iter) {
        if (isKnown(conclusion)) break;

        bool progress = false;

        progress |= applyMP();
        progress |= applyMT();
        progress |= applyHS();
        progress |= applyDS();
        progress |= applyCD();
        progress |= applyDD();
        progress |= applySimp();
        progress |= applyConj(conclusion);
        progress |= applyAdd (conclusion);

        progress |= applyDM();
        progress |= applyCom();
        progress |= applyAssoc();
        progress |= applyDist();
        progress |= applyDN();
        progress |= applyTrans();
        progress |= applyMImpl();
        progress |= applyMEquiv();
        progress |= applyExp();
        progress |= applyTaut();

        if (!progress) break;
    }

    if (isKnown(conclusion)) {
        return {true, kb_,
                "Argument is valid. Conclusion derived in " +
                std::to_string(kb_.size()) + " steps.", ""};
    }

    return {false, kb_,
            "Could not prove the conclusion. The argument may be invalid, "
            "or requires techniques beyond propositional logic.", ""};
}
