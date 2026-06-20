// backend/include/inference_engine.h
#pragma once
#include "formula.h"
#include <functional>
#include <string>
#include <vector>

struct ProofStep {
    int         stepNumber;
    FormulaPtr  formula;
    std::string formulaStr;
    std::string ruleName;
    std::vector<int> deps;
};

class InferenceEngine {
public:
    struct Result {
        bool                   proved;
        std::vector<ProofStep> steps;
        std::string            message;
        std::string            error;
    };

    Result prove(const std::vector<std::string>& premiseStrs,
                 const std::string&              conclusionStr,
                 int maxIterations = 150);

private:
    std::vector<ProofStep> kb_;

    bool isKnown(const FormulaPtr& f) const;
    bool addToKB(FormulaPtr f, const std::string& rule,
                 const std::vector<int>& deps);

    bool applyMP();
    bool applyMT();
    bool applyHS();
    bool applyDS();
    bool applyCD();
    bool applyDD();
    bool applySimp();
    bool applyConj(const FormulaPtr& goal);
    bool applyAdd (const FormulaPtr& goal);

    bool applyDM();
    bool applyCom();
    bool applyAssoc();
    bool applyDist();
    bool applyDN();
    bool applyTrans();
    bool applyMImpl();
    bool applyMEquiv();
    bool applyExp();
    bool applyTaut();

    std::vector<FormulaPtr> transformRecursive(
        FormulaPtr f,
        const std::function<FormulaPtr(FormulaPtr)>& transform);

    bool applyReplacement(const std::string& name,
                          const std::function<FormulaPtr(FormulaPtr)>& transform);

    void collectByType(FormulaPtr f, FormulaType t,
                       std::vector<FormulaPtr>& out) const;
};
