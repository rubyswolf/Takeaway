#include "strategy.h"

#include <fstream>
#include <locale.h>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
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

Strategy buildUniversalishStrategy() {
    StrategyBuilder builder;

#include "universalish strategy.cpp"

    return builder.finish();
}

struct ExactHistoryNode : ConditionNode {
    Game expected;

    explicit ExactHistoryNode(const Game& expected) : expected(expected) {}

    bool eval(const Game& game) const override {
        if (game.E.size != expected.E.size || game.size() != expected.size()) {
            return false;
        }

        for (int i = 0; i < static_cast<int>(game.size()); i++) {
            if (game[i] != expected[i]) {
                return false;
            }
        }

        return true;
    }
};

struct ExactMoveNode : MoveTestNode {
    Move expected;

    explicit ExactMoveNode(Move expected) : expected(expected) {}

    bool eval(const Game&, Move move) const override {
        return move == expected;
    }
};

Condition exact_history(const Game& game) {
    return Condition{ std::make_shared<ExactHistoryNode>(game) };
}

MoveTest exact_move(Move move) {
    return MoveTest{ std::make_shared<ExactMoveNode>(move) };
}

struct Correction {
    Game history;
    Move move;
};

Strategy withCorrections(const Strategy& base, const std::vector<Correction>& corrections) {
    Strategy corrected;

    for (int i = 0; i < static_cast<int>(corrections.size()); i++) {
        corrected.rules.push_back({
            exact_history(corrections[i].history),
            exact_move(corrections[i].move),
            "Correction " + std::to_string(i + 1)
        });
    }

    corrected.rules.insert(corrected.rules.end(), base.rules.begin(), base.rules.end());
    return corrected;
}

void printLine(const std::vector<Move>& line, UniversalSet E, std::ostream& out = std::cout) {
    Game position{ E };
    bool isPlayerOnesTurn = true;

    for (Move move : line) {
        out << "  " << ManipulateMove::moveLine(
            E,
            move,
            static_cast<int>(position.size()) + 1,
            isPlayerOnesTurn
        ) << '\n';
        position.playMove(move);
        isPlayerOnesTurn = !isPlayerOnesTurn;
    }
}

std::optional<Correction> findEarliestCorrection(
    const Strategy&,
    const Game&,
    const std::vector<Move>&,
    bool) {
    return std::nullopt;
}

struct SearchState {
    Strategy baseStrategy;
    Game start;
    bool strategyPlayersTurn = false;
};

std::string historyKey(const Game& game) {
    std::ostringstream out;
    out << game.E.size;
    for (Move move : game) {
        out << '|' << move;
    }
    return out.str();
}

std::string correctionsKey(const std::vector<Correction>& corrections) {
    std::ostringstream out;
    for (const Correction& correction : corrections) {
        out << '[' << historyKey(correction.history) << "->" << correction.move << ']';
    }
    return out.str();
}

bool sameHistory(const Game& a, const Game& b) {
    if (a.E.size != b.E.size || a.size() != b.size()) {
        return false;
    }

    for (int i = 0; i < static_cast<int>(a.size()); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

bool canAddCorrection(const std::vector<Correction>& corrections, const Correction& candidate) {
    for (const Correction& existing : corrections) {
        if (!sameHistory(existing.history, candidate.history)) {
            continue;
        }

        return existing.move == candidate.move;
    }

    return true;
}

struct StrategyTurnOnLine {
    Game history;
    Move chosenMove = 0;
};

std::vector<StrategyTurnOnLine> strategyTurnsOnCounterexample(
    const Game& start,
    const std::vector<Move>& line,
    bool strategyPlayersTurn) {

    std::vector<StrategyTurnOnLine> turns;
    Game position = start;
    bool playerOnesTurn = true;

    for (Move move : line) {
        if (playerOnesTurn == strategyPlayersTurn) {
            turns.push_back({ position, move });
        }

        position.playMove(move);
        playerOnesTurn = !playerOnesTurn;
    }

    return turns;
}

bool sameLine(const std::vector<Move>& a, const std::vector<Move>& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (int i = 0; i < static_cast<int>(a.size()); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

int commonPrefixLength(const std::vector<Move>& a, const std::vector<Move>& b) {
    const int limit = std::min(static_cast<int>(a.size()), static_cast<int>(b.size()));
    int prefix = 0;

    while (prefix < limit && a[prefix] == b[prefix]) {
        prefix++;
    }

    return prefix;
}

std::vector<Correction> appendCorrections(
    const std::vector<Correction>& existing,
    const std::vector<Correction>& extra) {

    std::vector<Correction> combined = existing;
    combined.insert(combined.end(), extra.begin(), extra.end());
    return combined;
}

struct CandidatePatch {
    std::vector<Correction> corrections;
    StrategyVerificationResult result;
    int depth = 0;
    int branchOrder = 0;
    int sharedPrefix = 0;
    int earliestCorrectedMoveNumber = 0;
};

int earliestCorrectedMoveNumber(const std::vector<Correction>& corrections) {
    int earliest = 0;

    for (const Correction& correction : corrections) {
        const int moveNumber = static_cast<int>(correction.history.size()) + 1;
        if (earliest == 0 || moveNumber < earliest) {
            earliest = moveNumber;
        }
    }

    return earliest;
}

bool isBetterCandidate(const CandidatePatch& candidate, const CandidatePatch& best) {
    if (candidate.result.wins != best.result.wins) {
        return candidate.result.wins;
    }

    if (candidate.sharedPrefix != best.sharedPrefix) {
        return candidate.sharedPrefix > best.sharedPrefix;
    }

    if (candidate.result.line.size() != best.result.line.size()) {
        return candidate.result.line.size() > best.result.line.size();
    }

    if (candidate.earliestCorrectedMoveNumber != best.earliestCorrectedMoveNumber) {
        return candidate.earliestCorrectedMoveNumber > best.earliestCorrectedMoveNumber;
    }

    if (candidate.depth != best.depth) {
        return candidate.depth < best.depth;
    }

    if (candidate.branchOrder != best.branchOrder) {
        return candidate.branchOrder < best.branchOrder;
    }

    return correctionsKey(candidate.corrections) < correctionsKey(best.corrections);
}

void considerCandidatePatch(
    const SearchState& state,
    const std::vector<Correction>& existingCorrections,
    const std::vector<Move>& targetLine,
    const std::vector<Correction>& extraCorrections,
    int depth,
    int branchOrder,
    std::optional<CandidatePatch>& bestPatch) {

    const Strategy correctedStrategy =
        withCorrections(state.baseStrategy, appendCorrections(existingCorrections, extraCorrections));
    const StrategyVerificationResult result =
        verifyStrategy(correctedStrategy, state.start, state.strategyPlayersTurn);

    if (hasStrategyRuntimeError()) {
        return;
    }

    if (!result.wins && sameLine(result.line, targetLine)) {
        return;
    }

    CandidatePatch candidate;
    candidate.corrections = extraCorrections;
    candidate.result = result;
    candidate.depth = depth;
    candidate.branchOrder = branchOrder;
    candidate.sharedPrefix = commonPrefixLength(result.line, targetLine);
    candidate.earliestCorrectedMoveNumber = earliestCorrectedMoveNumber(extraCorrections);

    if (!bestPatch.has_value() || isBetterCandidate(candidate, *bestPatch)) {
        bestPatch = std::move(candidate);
    }
}

void searchChosenTurnCorrections(
    const SearchState& state,
    const std::vector<Correction>& existingCorrections,
    const std::vector<Move>& targetLine,
    const std::vector<StrategyTurnOnLine>& chosenTurns,
    int turnIndex,
    std::vector<Correction>& extraCorrections,
    int depth,
    int branchOrder,
    std::optional<CandidatePatch>& bestPatch) {

    if (turnIndex == static_cast<int>(chosenTurns.size())) {
        considerCandidatePatch(
            state,
            existingCorrections,
            targetLine,
            extraCorrections,
            depth,
            branchOrder,
            bestPatch
        );
        return;
    }

    const StrategyTurnOnLine& turn = chosenTurns[turnIndex];
    for (Move candidate : turn.history.legalMoves()) {
        if (candidate == turn.chosenMove) {
            continue;
        }

        const Correction correction{ turn.history, candidate };
        if (!canAddCorrection(existingCorrections, correction) || !canAddCorrection(extraCorrections, correction)) {
            continue;
        }

        extraCorrections.push_back(correction);
        searchChosenTurnCorrections(
            state,
            existingCorrections,
            targetLine,
            chosenTurns,
            turnIndex + 1,
            extraCorrections,
            depth,
            branchOrder,
            bestPatch
        );
        extraCorrections.pop_back();
    }
}

void chooseTurnCombination(
    const SearchState& state,
    const std::vector<Correction>& existingCorrections,
    const std::vector<Move>& targetLine,
    const std::vector<StrategyTurnOnLine>& windowTurns,
    int startIndex,
    int remainingChoices,
    std::vector<StrategyTurnOnLine>& chosenTurns,
    std::vector<Correction>& extraCorrections,
    int depth,
    int branchOrder,
    std::optional<CandidatePatch>& bestPatch) {

    if (remainingChoices == 0) {
        searchChosenTurnCorrections(
            state,
            existingCorrections,
            targetLine,
            chosenTurns,
            0,
            extraCorrections,
            depth,
            branchOrder,
            bestPatch
        );
        return;
    }

    const int turnsRemaining = static_cast<int>(windowTurns.size()) - startIndex;
    if (turnsRemaining < remainingChoices) {
        return;
    }

    for (int i = startIndex; i < static_cast<int>(windowTurns.size()); i++) {
        chosenTurns.push_back(windowTurns[i]);
        chooseTurnCombination(
            state,
            existingCorrections,
            targetLine,
            windowTurns,
            i + 1,
            remainingChoices - 1,
            chosenTurns,
            extraCorrections,
            depth,
            branchOrder,
            bestPatch
        );
        chosenTurns.pop_back();
    }
}

void writeCorrectionsFile(
    const std::string& path,
    UniversalSet E,
    const std::vector<Correction>& corrections) {

    std::ofstream out(path);
    for (const Correction& correction : corrections) {
        for (int i = 0; i < static_cast<int>(correction.history.size()); i++) {
            if (i != 0) {
                out << ",";
            }
            out << correction.history[i];
        }
        if (!correction.history.empty()) {
            out << ",";
        }
        out << correction.move << "\n";
    }
}
} // namespace

int main() {
    configureConsoleForUnicode();

    constexpr int n = 5;
    constexpr bool strategyPlayersTurn = false; // Strategy is player 2 from the starting position
    constexpr int maxPatchDepth = 4;
    constexpr int maxBranchOrder = 3;

    const UniversalSet E{ n };
    const Game start{ E };
    const SearchState state{ buildUniversalishStrategy(), start, strategyPlayersTurn };
    std::vector<Correction> corrections;
    int counterexampleIndex = 0;

    for (;;) {
        const Strategy correctedStrategy = withCorrections(state.baseStrategy, corrections);
        const StrategyVerificationResult result =
            verifyStrategy(correctedStrategy, state.start, state.strategyPlayersTurn);

        if (hasStrategyRuntimeError()) {
            std::cout << "\nStopped because the strategy runtime failed.\n";
            return 1;
        }

        if (result.wins) {
            std::cout << "Found a full correction table for n=" << n << ".\n";
            std::cout << "Total corrections: " << corrections.size() << "\n\n";
            break;
        }

        counterexampleIndex++;
        std::cout << "Counterexample " << counterexampleIndex
                  << " with " << corrections.size() << " corrections so far:\n";
        printLine(result.line, E);
        std::cout << '\n';

        const std::vector<StrategyTurnOnLine> targetTurns =
            strategyTurnsOnCounterexample(state.start, result.line, state.strategyPlayersTurn);

        bool patched = false;
        std::optional<CandidatePatch> bestPatch;

        for (int depth = 1; depth <= maxPatchDepth && !patched; depth++) {
            if (depth > static_cast<int>(targetTurns.size())) {
                continue;
            }

            for (int branchOrder = 1; branchOrder <= depth && branchOrder <= maxBranchOrder; branchOrder++) {
                std::vector<Correction> extraCorrections;

                if (branchOrder == 1) {
                    std::cout << "  Trying patch depth " << depth << "...\n";

                    const int turnIndexFromStart = static_cast<int>(targetTurns.size()) - depth;
                    const std::vector<StrategyTurnOnLine> chosenTurns{ targetTurns[turnIndexFromStart] };
                    searchChosenTurnCorrections(
                        state,
                        corrections,
                        result.line,
                        chosenTurns,
                        0,
                        extraCorrections,
                        depth,
                        branchOrder,
                        bestPatch
                    );
                }
                else {
                    std::vector<StrategyTurnOnLine> chosenTurns;
                    const int windowStart = static_cast<int>(targetTurns.size()) - depth;
                    const std::vector<StrategyTurnOnLine> windowTurns(
                        targetTurns.begin() + windowStart,
                        targetTurns.end()
                    );

                    std::cout << "  Trying branching order " << branchOrder
                              << " at depth " << depth << "...\n";
                    chooseTurnCombination(
                        state,
                        corrections,
                        result.line,
                        windowTurns,
                        0,
                        branchOrder,
                        chosenTurns,
                        extraCorrections,
                        depth,
                        branchOrder,
                        bestPatch
                    );
                }

                if (bestPatch.has_value() && bestPatch->result.wins) {
                    patched = true;
                }
            }
        }

        if (!bestPatch.has_value()) {
            std::cout << "Failed to patch counterexample " << counterexampleIndex
                      << " up to depth " << maxPatchDepth
                      << " and branching order " << maxBranchOrder << ".\n";
            return 1;
        }

        std::cout << "  Chose depth " << bestPatch->depth
                  << " with branching order " << bestPatch->branchOrder << ".\n";
        std::cout << "  Added " << bestPatch->corrections.size() << " correction";
        if (bestPatch->corrections.size() != 1) {
            std::cout << "s";
        }
        std::cout << ".\n";
        for (int i = 0; i < static_cast<int>(bestPatch->corrections.size()); i++) {
            const Correction& correction = bestPatch->corrections[i];
            std::cout
                << "    "
                << ManipulateMove::moveLine(
                    E,
                    correction.move,
                    static_cast<int>(correction.history.size()) + 1,
                    (correction.history.size() % 2 == 0),
                    "Correction " + std::to_string(corrections.size() + i + 1)
                )
                << '\n';
        }
        if (bestPatch->result.wins) {
            std::cout << "  This patch wins from the starting position.\n\n";
        }
        else {
            std::cout << "  New first counterexample after this patch:\n";
            printLine(bestPatch->result.line, E);
            std::cout << '\n';
        }

        corrections.insert(
            corrections.end(),
            bestPatch->corrections.begin(),
            bestPatch->corrections.end()
        );
    }

    const std::string outputPath = "n" + std::to_string(n) + " universalish corrections.txt";
    writeCorrectionsFile(outputPath, E, corrections);
    std::cout << "Corrections written to " << outputPath << "\n";
    return 0;
}
