#include "strategy.h"

#include <future>
#include <locale.h>
#include <iostream>
#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace {
void configureConsoleForUnicode() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
}
}

Strategy buildN3WinningStrategy() {
    StrategyBuilder builder;

#include "n3 winning strategy.cpp"

    return builder.finish();
}

Strategy buildN4WinningStrategy() {
    StrategyBuilder builder;

#include "n4 winning strategy.cpp"

    return builder.finish();
}

Strategy buildUniversalishStrategy() {
    StrategyBuilder builder;

#include "universalish strategy.cpp"

    return builder.finish();
}

void printCounterexampleLine(
    const Game& start,
    const std::vector<Move>& line,
    const Strategy& strategy,
    bool strategyPlayersTurn) {

    Game position = start;
    bool playerOnesTurn = true;

    for (Move move : line) {
        const std::optional<std::string> ruleName =
            (playerOnesTurn == strategyPlayersTurn)
            ? ruleNameForMove(strategy, position, move)
            : std::nullopt;

        std::cout
            << "  "
            << ManipulateMove::moveLine(
                position.E,
                move,
                static_cast<int>(position.size()) + 1,
                playerOnesTurn,
                ruleName
            )
            << '\n';

        position.playMove(move);
        playerOnesTurn = !playerOnesTurn;
    }
}

void testStrategy(const std::string& label, int n, const Strategy& strategy) {
    std::cout << label << ":\n";

    Game start{ UniversalSet(n) };
    const StrategyVerificationResult result = verifyStrategyParallel(strategy, start, false);

    if (hasStrategyRuntimeError()) {
        std::cout << '\n';
        return;
    }

    if (result.wins) {
        std::cout << "  winning strategy\n";
    }
    else {
        std::cout << "  counterexample found\n";
        printCounterexampleLine(start, result.line, strategy, false);
    }

    std::cout << '\n';
}

struct TestRunResult {
    std::string label;
    int n = 0;
    Strategy strategy;
    StrategyVerificationResult result;
    bool hadRuntimeError = false;
};

TestRunResult runTestCase(const std::string& label, int n, Strategy strategy) {
    Game start{ UniversalSet(n) };
    const StrategyVerificationResult result = verifyStrategyParallel(strategy, start, false);
    return {
        label,
        n,
        std::move(strategy),
        result,
        hasStrategyRuntimeError()
    };
}

void printTestRunResult(const TestRunResult& run) {
    std::cout << run.label << ":\n";

    if (run.hadRuntimeError) {
        std::cout << '\n';
        return;
    }

    Game start{ UniversalSet(run.n) };
    if (run.result.wins) {
        std::cout << "  winning strategy\n";
    }
    else {
        std::cout << "  counterexample found\n";
        printCounterexampleLine(start, run.result.line, run.strategy, false);
    }

    std::cout << '\n';
}

int main() {
    configureConsoleForUnicode();

    std::vector<std::future<TestRunResult>> futures;
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("winning n=3", 3, buildN3WinningStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("winning n=4", 4, buildN4WinningStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=3", 3, buildUniversalishStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=4", 4, buildUniversalishStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=5", 5, buildUniversalishStrategy()); }));

    for (auto& future : futures) {
        printTestRunResult(future.get());
    }
}
