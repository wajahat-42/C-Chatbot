// backend/src/server.cpp
#include "crow.h"
#include "inference_engine.h"
#include <string>
#include <vector>

int main() {
    crow::SimpleApp app;
    InferenceEngine engine;

    // ── CORS preflight ────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/prove").methods("OPTIONS"_method)(
        [](const crow::request&) {
            crow::response res(204);
            res.add_header("Access-Control-Allow-Origin",  "*");
            res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.add_header("Access-Control-Allow-Headers", "Content-Type");
            return res;
        }
    );

    // ── Main endpoint ─────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/prove").methods("POST"_method)(
        [&engine](const crow::request& req) {
            crow::response res;
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Content-Type",                "application/json");

            auto body = crow::json::load(req.body);
            if (!body) {
                res.code = 400;
                res.body = R"({"error":"Invalid JSON in request body"})";
                return res;
            }

            std::vector<std::string> premises;
            if (body.has("premises")) {
                const auto& arr = body["premises"];
                for (std::size_t i = 0; i < arr.size(); ++i)
                    premises.push_back(arr[i].s());
            }

            if (!body.has("conclusion")) {
                res.code = 400;
                res.body = R"({"error":"Missing field: conclusion"})";
                return res;
            }
            std::string conclusion = body["conclusion"].s();

            int maxIter = 150;
            if (body.has("options") && body["options"].has("maxIterations"))
                maxIter = body["options"]["maxIterations"].i();

            auto result = engine.prove(premises, conclusion, maxIter);

            crow::json::wvalue resp;
            resp["proved"]  = result.proved;
            resp["message"] = result.message;
            resp["error"]   = result.error;

            std::vector<crow::json::wvalue> stepsArr;
            stepsArr.reserve(result.steps.size());

            for (const auto& step : result.steps) {
                crow::json::wvalue s;
                s["step"]    = step.stepNumber;
                s["formula"] = step.formulaStr;
                s["rule"]    = step.ruleName;

                std::vector<crow::json::wvalue> depsArr;
                for (int d : step.deps) {
                    crow::json::wvalue dv;
                    dv = d;
                    depsArr.push_back(std::move(dv));
                }
                s["deps"] = std::move(depsArr);
                stepsArr.push_back(std::move(s));
            }

            resp["steps"] = std::move(stepsArr);
            res.code = result.error.empty() ? 200 : 422;
            res.body = resp.dump();
            return res;
        }
    );

    // ── Health-check ──────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/health")([]{
        return crow::response(200, R"({"status":"ok"})");
    });

    CROW_LOG_INFO << "Logic Engine running on http://localhost:8080";
    app.port(8080).multithreaded().run();
}
