#include "strategy.h"

#include <locale.h>
#include <iostream>
#include <string>
#include <vector>
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

Strategy buildUniversalStrategy() {
    StrategyBuilder builder;

#include "universal strategy.cpp"

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
    const StrategyVerificationResult result = verifyStrategy(strategy, start, false);

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

int main() {
    configureConsoleForUnicode();
    testStrategy("winning n=3", 3, buildN3WinningStrategy());
    testStrategy("winning n=4", 4, buildN4WinningStrategy());
    testStrategy("universal n=3", 3, buildUniversalStrategy());
    testStrategy("universal n=4", 4, buildUniversalStrategy());
}
