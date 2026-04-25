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

#ifndef VERIFY_ROOT_SLICE_COUNT
#define VERIFY_ROOT_SLICE_COUNT 1
#endif

#ifndef VERIFY_ROOT_SLICE_INDEX
#define VERIFY_ROOT_SLICE_INDEX 0
#endif

constexpr bool kSplitRootVerification = VERIFY_ROOT_SLICE_COUNT > 1;
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

Strategy buildN5WinningStrategy() {
    StrategyBuilder builder;

#include "n5 winning strategy.cpp"

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
    bool splitRootVerification = false;
    std::size_t sliceIndex = 0;
    std::size_t sliceCount = 1;
};

TestRunResult runTestCase(const std::string& label, int n, Strategy strategy) {
    Game start{ UniversalSet(n) };
    const StrategyVerificationResult result = verifyStrategyParallel(strategy, start, false);
    return {
        label,
        n,
        std::move(strategy),
        result,
        hasStrategyRuntimeError(),
        false,
        0,
        1
    };
}

TestRunResult runSplitTestCase(
    const std::string& label,
    int n,
    Strategy strategy,
    std::size_t sliceIndex,
    std::size_t sliceCount) {

    Game start{ UniversalSet(n) };
    const StrategyVerificationResult result =
        verifyStrategyParallelSlice(strategy, start, false, sliceIndex, sliceCount);

    return {
        label,
        n,
        std::move(strategy),
        result,
        hasStrategyRuntimeError(),
        true,
        sliceIndex,
        sliceCount
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
        if (run.splitRootVerification) {
            std::cout
                << "  no counterexample found in assigned root slice "
                << (run.sliceIndex + 1)
                << '/'
                << run.sliceCount
                << '\n';
        }
        else {
            std::cout << "  winning strategy\n";
        }
    }
    else {
        if (run.splitRootVerification) {
            std::cout
                << "  counterexample found in assigned root slice "
                << (run.sliceIndex + 1)
                << '/'
                << run.sliceCount
                << '\n';
        }
        else {
            std::cout << "  counterexample found\n";
        }
        printCounterexampleLine(start, run.result.line, run.strategy, false);
    }

    std::cout << '\n';
}

int main() {
    configureConsoleForUnicode();

    if constexpr (kSplitRootVerification) {
        const std::string label =
            "universalish n=5 root slice "
            + std::to_string(VERIFY_ROOT_SLICE_INDEX + 1)
            + "/"
            + std::to_string(VERIFY_ROOT_SLICE_COUNT);
        printTestRunResult(
            runSplitTestCase(
                label,
                5,
                buildUniversalishStrategy(),
                VERIFY_ROOT_SLICE_INDEX,
                VERIFY_ROOT_SLICE_COUNT));
        return 0;
    }

    std::vector<std::future<TestRunResult>> futures;
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("winning n=3", 3, buildN3WinningStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("winning n=4", 4, buildN4WinningStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("winning n=5", 5, buildN5WinningStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=3", 3, buildUniversalishStrategy()); }));
    futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=4", 4, buildUniversalishStrategy()); }));
    //futures.push_back(std::async(std::launch::async, []() { return runTestCase("universalish n=5", 5, buildUniversalishStrategy()); }));

    for (auto& future : futures) {
        printTestRunResult(future.get());
    }
}
