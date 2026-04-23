#include "strategy.h"

#include <iostream>
#include <string>
#include <vector>

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

void printCounterexampleLine(const Game& start, const std::vector<Move>& line) {
    Game position = start;
    bool playerOnesTurn = true;

    for (Move move : line) {
        std::cout
            << "  P" << (playerOnesTurn ? 1 : 2)
            << ": " << ManipulateMove::toString(position.E, move)
            << '\n';

        position.playMove(move);
        playerOnesTurn = !playerOnesTurn;
    }
}

void testStrategy(const std::string& label, int n, const Strategy& strategy) {
    Game start{ UniversalSet(n) };
    StrategyVerificationResult result = verifyStrategy(strategy, start, false);

    std::cout << label << ":\n";
    if (result.wins) {
        std::cout << "  winning strategy\n";
    }
    else {
        std::cout << "  counterexample found\n";
        printCounterexampleLine(start, result.line);
    }

    std::cout << '\n';
}

int main() {
    testStrategy("n=3", 3, buildN3WinningStrategy());
    testStrategy("n=4", 4, buildN4WinningStrategy());
    testStrategy("universal n=3", 3, buildUniversalStrategy());
    testStrategy("universal n=4", 4, buildUniversalStrategy());
}
