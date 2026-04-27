#include "strategy.h"

// Pick the strategy scripts you want to be able to assign player one and two
#define strategyScriptFor1 "universalish strategy.cpp"
#define strategyScriptFor2 "universalish strategy.cpp"

#include <conio.h>
#include <cctype>
#include <cstdlib>
#include <locale.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace {
enum class PlayerMode {
    Manual,
    Strategy,
    SetResponse
};

const Strategy* g_playerOneStrategy = nullptr;
const Strategy* g_playerTwoStrategy = nullptr;
std::vector<std::optional<std::string>> g_moveRuleNames;

struct SetResponseRule {
    std::vector<Move> requiredMoves;
    Move response = 0;
    std::string text;
};

struct SetResponseTable {
    int n = 0;
    std::vector<SetResponseRule> rules;
};

std::optional<SetResponseTable> g_setResponseTable;
std::optional<std::string> g_lastSetResponseRuleName;

Strategy buildCustomStrategy1() {
    StrategyBuilder builder;

#include strategyScriptFor1

    return builder.finish();
}

Strategy buildCustomStrategy2() {
    StrategyBuilder builder;

#include strategyScriptFor2

    return builder.finish();
}

void configureConsoleForUnicode() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
}

void clearScreen() {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE) {
        std::system("cls");
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(console, &info)) {
        std::system("cls");
        return;
    }

    const DWORD cellCount = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
    DWORD written = 0;
    COORD home{ 0, 0 };

    FillConsoleOutputCharacterA(console, ' ', cellCount, home, &written);
    FillConsoleOutputAttribute(console, info.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(console, home);
}

std::string modeLabel(PlayerMode mode, std::string strategyScript) {
    if (mode == PlayerMode::Manual) {
        return "Manual";
    }
    if (mode == PlayerMode::SetResponse) {
        return "Set response";
    }

    return "Strategy (" + strategyScript + ")";
}

PlayerMode choosePlayerMode(bool isPlayerOne) {
    int selection = 0;
    const int optionCount = isPlayerOne ? 2 : 3;

    while (true) {
        clearScreen();
        std::cout << "Select Player " << (isPlayerOne ? "1" : "2") << " mode\n\n";
        std::cout << (selection == 0 ? "> " : "  ") << "Manual\n";
        std::cout << (selection == 1 ? "> " : "  ") << "Strategy (" << (isPlayerOne ? strategyScriptFor1 : strategyScriptFor2) << ")\n";
        if (!isPlayerOne) {
            std::cout << (selection == 2 ? "> " : "  ") << "Set response\n";
        }
        std::cout << "\nUse Up/Down arrows and Enter.\n";

        const int key = _getch();
        if (key == 224 || key == 0) {
            const int arrow = _getch();
            if (arrow == 72 || arrow == 75) {
                selection = (selection + optionCount - 1) % optionCount;
            }
            else if (arrow == 80 || arrow == 77) {
                selection = (selection + 1) % optionCount;
            }
        }
        else if (key == 13) {
            if (selection == 0) return PlayerMode::Manual;
            if (selection == 1) return PlayerMode::Strategy;
            return PlayerMode::SetResponse;
        }
    }
}

int promptUniversalSetSize() {
    while (true) {
        clearScreen();
        std::cout << "Enter the size n of the universal set E: ";

        std::string line;
        std::getline(std::cin, line);

        try {
            const int n = std::stoi(line);
            if (n >= 2 && n <= 30) {
                return n;
            }
        }
        catch (...) {
        }

        std::cout << "\nPlease enter an integer from 2 to 30.\n";
        std::cout << "Press any key to continue.";
        _getch();
    }
}

std::vector<std::string> historyLines(const Game& game, PlayerMode playerOneMode, PlayerMode playerTwoMode) {
    std::vector<std::string> lines;
    bool isPlayerOnesTurn = true;
    int moveNumber = 1;
    Game position{ game.E };

    for (Move move : game) {
        std::optional<std::string> ruleName = std::nullopt;

        if (moveNumber - 1 < static_cast<int>(g_moveRuleNames.size()) && g_moveRuleNames[moveNumber - 1].has_value()) {
            ruleName = g_moveRuleNames[moveNumber - 1];
        }
        else if (isPlayerOnesTurn && playerOneMode == PlayerMode::Strategy && g_playerOneStrategy != nullptr) {
            ruleName = ruleNameForMove(*g_playerOneStrategy, position, move);
        }
        else if (!isPlayerOnesTurn && playerTwoMode == PlayerMode::Strategy && g_playerTwoStrategy != nullptr) {
            ruleName = ruleNameForMove(*g_playerTwoStrategy, position, move);
        }

        lines.push_back(ManipulateMove::moveLine(game.E, move, moveNumber, isPlayerOnesTurn, ruleName));
        position.playMove(move);
        isPlayerOnesTurn = !isPlayerOnesTurn;
        moveNumber++;
    }

    return lines;
}

void renderGame(
    const Game& game,
    PlayerMode playerOneMode,
    PlayerMode playerTwoMode,
    const std::string& prompt = "",
    const std::string& message = "") {

    clearScreen();

    const bool isPlayerOnesTurn = (game.size() % 2 == 0);
    const int currentPlayer = isPlayerOnesTurn ? 1 : 2;

    std::cout << "Takeaway\n";
    std::cout << "E size: " << game.E.size << "\n";
    std::cout << "Player 1: " << modeLabel(playerOneMode, strategyScriptFor1) << "\n";
    std::cout << "Player 2: " << modeLabel(playerTwoMode, strategyScriptFor2) << "\n";
    std::cout << "Current player: P" << currentPlayer << "\n\n";

    std::cout << "History:\n";
    const std::vector<std::string> lines = historyLines(game, playerOneMode, playerTwoMode);
    if (lines.empty()) {
        std::cout << "  (empty)\n";
    }
    else {
        for (const std::string& line : lines) {
            std::cout << "  " << line << "\n";
        }
    }

    std::cout << "\n";
    if (!message.empty()) {
        std::cout << message << "\n\n";
    }
    if (!prompt.empty()) {
        std::cout << prompt;
    }
}

std::optional<Move> parseSetNotation(const std::string& input, UniversalSet E) {
    if (input.size() < 2 || input.front() != '{' || input.back() != '}') {
        return std::nullopt;
    }

    const std::string inner = input.substr(1, input.size() - 2);
    if (inner.find_first_not_of(" \t") == std::string::npos) {
        return 0;
    }

    std::stringstream stream(inner);
    std::string token;
    Move move = 0;

    while (std::getline(stream, token, ',')) {
        std::stringstream tokenStream(token);
        int element = 0;
        tokenStream >> element;

        if (!tokenStream || element < 1 || element > E.size) {
            return std::nullopt;
        }

        tokenStream >> std::ws;
        if (!tokenStream.eof()) {
            return std::nullopt;
        }

        move |= 1 << (element - 1);
    }

    return move;
}

std::optional<Move> parseMoveInput(const std::string& input, UniversalSet E) {
    if (input.empty()) {
        return std::nullopt;
    }

    if (input.front() == '{') {
        return parseSetNotation(input, E);
    }

    try {
        const int move = std::stoi(input);
        if (move < 0 || move > E.bitmask) {
            return std::nullopt;
        }
        return move;
    }
    catch (...) {
        return std::nullopt;
    }
}

std::string trim(const std::string& input) {
    const std::size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

std::optional<SetResponseRule> parseSetResponseRule(const std::string& line) {
    const std::size_t arrow = line.find("->");
    if (arrow == std::string::npos) {
        return std::nullopt;
    }

    std::string left = trim(line.substr(0, arrow));
    std::string right = trim(line.substr(arrow + 2));
    if (left.size() < 2 || left.front() != '{' || left.back() != '}') {
        return std::nullopt;
    }

    SetResponseRule rule;
    rule.text = left + " -> " + right;

    const std::string inner = trim(left.substr(1, left.size() - 2));
    if (!inner.empty()) {
        std::stringstream stream(inner);
        std::string token;
        while (std::getline(stream, token, ',')) {
            try {
                rule.requiredMoves.push_back(std::stoi(trim(token)));
            }
            catch (...) {
                return std::nullopt;
            }
        }
    }

    try {
        rule.response = std::stoi(right);
    }
    catch (...) {
        return std::nullopt;
    }

    return rule;
}

bool loadSetResponseTable(int n, std::string& error) {
    if (g_setResponseTable.has_value() && g_setResponseTable->n == n) {
        return true;
    }

    const std::string fileName = "n" + std::to_string(n) + " set response.txt";
    std::ifstream file(fileName);
    if (!file) {
        error = "Could not find set response file: " + fileName;
        return false;
    }

    SetResponseTable table;
    table.n = n;

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::optional<SetResponseRule> rule = parseSetResponseRule(line);
        if (!rule.has_value()) {
            error = "Invalid set response rule on line " + std::to_string(lineNumber) + " of " + fileName;
            return false;
        }

        table.rules.push_back(*rule);
    }

    g_setResponseTable = table;
    return true;
}

std::optional<Move> chooseSetResponseMove(const Game& game, std::string& error) {
    if (!loadSetResponseTable(game.E.size, error)) {
        return std::nullopt;
    }

    std::vector<int> permutation;
    for (int element = 0; element < game.E.size; element++) {
        permutation.push_back(element);
    }

    for (const SetResponseRule& rule : g_setResponseTable->rules) {
        std::sort(permutation.begin(), permutation.end());
        do {
            std::vector<Move> gameMoves(game.begin(), game.end());
            std::sort(gameMoves.begin(), gameMoves.end());

            for (const SetResponseRule& rule : g_setResponseTable->rules) {
                if (rule.requiredMoves.size() != gameMoves.size()) {
                    continue;
                }

                std::vector<int> permutation;
                for (int element = 0; element < game.E.size; element++) {
                    permutation.push_back(element);
                }

                do {
                    std::vector<Move> relabelledRequiredMoves;
                    relabelledRequiredMoves.reserve(rule.requiredMoves.size());

                    for (Move requiredMove : rule.requiredMoves) {
                        relabelledRequiredMoves.push_back(
                            MoveNodeEquivalence::relabelMove(requiredMove, permutation, game.E)
                        );
                    }

                    std::sort(relabelledRequiredMoves.begin(), relabelledRequiredMoves.end());

                    if (relabelledRequiredMoves != gameMoves) {
                        continue;
                    }

                    Move response = MoveNodeEquivalence::relabelMove(rule.response, permutation, game.E);
                    if (game.isMoveLegal(response)) {
                        g_lastSetResponseRuleName = "Set response " + rule.text;
                        return response;
                    }
                } while (std::next_permutation(permutation.begin(), permutation.end()));
            }

            Move response = MoveNodeEquivalence::relabelMove(rule.response, permutation, game.E);
            if (game.isMoveLegal(response)) {
                g_lastSetResponseRuleName = "Set response " + rule.text;
                return response;
            }
        } while (std::next_permutation(permutation.begin(), permutation.end()));
    }

    error = "No set response rule matched this position.";
    return std::nullopt;
}

std::string normalizeCommand(std::string input) {
    for (char& c : input) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return input;
}

Move promptManualMove(const Game& game, PlayerMode& playerOneMode, PlayerMode& playerTwoMode) {
    const bool isPlayerOnesTurn = (game.size() % 2 == 0);
    std::string message;

    while (true) {
        renderGame(
            game,
            playerOneMode,
            playerTwoMode,
            "Enter a move as a set or a bitmask number: ",
            message);

        std::string input;
        std::getline(std::cin, input);
        const std::string command = normalizeCommand(input);

        if (command == "s" || command == "strategy") {
            const Strategy* strategy = isPlayerOnesTurn ? g_playerOneStrategy : g_playerTwoStrategy;
            const std::optional<Move> move = firstAllowedLegalMove(*strategy, game);
            if (!move.has_value()) {
                message = "The strategy has no legal move here.";
                continue;
            }
            const std::optional<std::string> ruleName = ruleNameForMove(*strategy, game, *move);
            g_moveRuleNames.push_back(ruleName);
            return *move;
        }

        if (command == "r" || command == "response") {
            std::string error;
            const std::optional<Move> move = chooseSetResponseMove(game, error);
            if (!move.has_value()) {
                message = error;
                continue;
            }

            g_moveRuleNames.push_back(g_lastSetResponseRuleName);
            return *move;
        }

        const std::optional<Move> move = parseMoveInput(input, game.E);
        if (!move.has_value()) {
            renderGame(game, playerOneMode, playerTwoMode, "", "Invalid input.");
            std::cout << "Press any key to continue.";
            _getch();
            message.clear();
            continue;
        }

        if (!game.isMoveLegal(*move)) {
            renderGame(game, playerOneMode, playerTwoMode, "", "That move is illegal.");
            std::cout << "Press any key to continue.";
            _getch();
            message.clear();
            continue;
        }

        g_moveRuleNames.push_back(std::nullopt);
        return *move;
    }
}

Move chooseStrategyMove(
    const Strategy& strategy,
    const Game& game,
    PlayerMode playerOneMode,
    PlayerMode playerTwoMode,
    bool isPlayerOnesTurn) {

    const std::optional<Move> move = firstAllowedLegalMove(strategy, game);
    if (!move.has_value()) {
        return -1;
    }

    const std::optional<std::string> ruleName = ruleNameForMove(strategy, game, *move);

    renderGame(
        game,
        playerOneMode,
        playerTwoMode,
        "",
        ManipulateMove::moveLine(
            game.E,
            *move,
            static_cast<int>(game.size()) + 1,
            isPlayerOnesTurn,
            ruleName
        ) + ".");

    return *move;
}

Move chooseSetResponseMoveForTurn(
    const Game& game,
    PlayerMode playerOneMode,
    PlayerMode playerTwoMode,
    bool isPlayerOnesTurn) {

    std::string error;
    const std::optional<Move> move = chooseSetResponseMove(game, error);
    if (!move.has_value()) {
        renderGame(game, playerOneMode, playerTwoMode, "", error);
        return -1;
    }

    renderGame(
        game,
        playerOneMode,
        playerTwoMode,
        "",
        ManipulateMove::moveLine(
            game.E,
            *move,
            static_cast<int>(game.size()) + 1,
            isPlayerOnesTurn,
            g_lastSetResponseRuleName
        ) + ".");

    return *move;
}
} // namespace

int main() {
    configureConsoleForUnicode();

    PlayerMode playerOneMode = choosePlayerMode(true);
    PlayerMode playerTwoMode = choosePlayerMode(false);

    const int n = promptUniversalSetSize();
    if (playerTwoMode == PlayerMode::SetResponse) {
        std::string error;
        if (!loadSetResponseTable(n, error)) {
            clearScreen();
            std::cout << error << "\n";
            std::cout << "Press any key to exit.";
            _getch();
            return 1;
        }
    }

    const Strategy playerOneStrategy = buildCustomStrategy1();
    const Strategy playerTwoStrategy = buildCustomStrategy2();
    g_playerOneStrategy = &playerOneStrategy;
    g_playerTwoStrategy = &playerTwoStrategy;
    g_moveRuleNames.clear();

    Game game{ UniversalSet(n) };

    while (true) {
        const bool isPlayerOnesTurn = (game.size() % 2 == 0);
        const PlayerMode mode = isPlayerOnesTurn ? playerOneMode : playerTwoMode;

        if (game.legalMoves().empty()) {
            renderGame(game, playerOneMode, playerTwoMode);
            std::cout << "Player " << (isPlayerOnesTurn ? 2 : 1) << " wins.\n";
            std::cout << "Press any key to exit.";
            _getch();
            return 0;
        }

        Move move = -1;
        if (mode == PlayerMode::Manual) {
            move = promptManualMove(game, playerOneMode, playerTwoMode);
        }
        else if (mode == PlayerMode::SetResponse) {
            move = chooseSetResponseMoveForTurn(game, playerOneMode, playerTwoMode, isPlayerOnesTurn);

            if (move == -1) {
                std::cout << "\nPress any key to exit.";
                _getch();
                return 1;
            }

            g_moveRuleNames.push_back(g_lastSetResponseRuleName);
        }
        else {
            const Strategy& strategy = isPlayerOnesTurn ? playerOneStrategy : playerTwoStrategy;
            move = chooseStrategyMove(strategy, game, playerOneMode, playerTwoMode, isPlayerOnesTurn);

            if (move == -1) {
                if (hasStrategyRuntimeError()) {
                    std::cout << "Press any key to exit.";
                    _getch();
                    return 0;
                }

                renderGame(game, playerOneMode, playerTwoMode);
                std::cout << "Player " << (isPlayerOnesTurn ? 2 : 1) << " wins.\n";
                std::cout << "Press any key to exit.";
                _getch();
                return 0;
            }

            g_moveRuleNames.push_back(ruleNameForMove(strategy, game, move));
        }

        game.playMove(move);
    }
}
