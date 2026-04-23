#include "strategy.h"

#include <conio.h>
#include <cstdlib>
#include <locale.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

namespace {
enum class PlayerMode {
    Manual,
    Strategy
};

Strategy buildCustomStrategy1() {
    StrategyBuilder builder;

#include "custom strategy 1.cpp"

    return builder.finish();
}

Strategy buildCustomStrategy2() {
    StrategyBuilder builder;

#include "custom strategy 2.cpp"

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

std::string modeLabel(PlayerMode mode, int playerNumber) {
    if (mode == PlayerMode::Manual) {
        return "Manual";
    }

    return "Strategy (custom strategy " + std::to_string(playerNumber) + ".cpp)";
}

PlayerMode choosePlayerMode(int playerNumber) {
    int selection = 0;

    while (true) {
        clearScreen();
        std::cout << "Select Player " << playerNumber << " mode\n\n";
        std::cout << (selection == 0 ? "> " : "  ") << "Manual\n";
        std::cout << (selection == 1 ? "> " : "  ") << "Strategy (custom strategy " << playerNumber << ".cpp)\n";
        std::cout << "\nUse Up/Down arrows and Enter.\n";

        const int key = _getch();
        if (key == 224 || key == 0) {
            const int arrow = _getch();
            if (arrow == 72 || arrow == 75) {
                selection = (selection + 1) % 2;
            }
            else if (arrow == 80 || arrow == 77) {
                selection = (selection + 1) % 2;
            }
        }
        else if (key == 13) {
            return selection == 0 ? PlayerMode::Manual : PlayerMode::Strategy;
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

std::vector<std::string> historyLines(const Game& game) {
    std::vector<std::string> lines;
    bool isPlayerOnesTurn = true;
    int moveNumber = 1;

    for (Move move : game) {
        lines.push_back(ManipulateMove::moveLine(game.E, move, moveNumber, isPlayerOnesTurn));
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
    std::cout << "Player 1: " << modeLabel(playerOneMode, 1) << "\n";
    std::cout << "Player 2: " << modeLabel(playerTwoMode, 2) << "\n";
    std::cout << "Current player: P" << currentPlayer << "\n\n";

    std::cout << "History:\n";
    const std::vector<std::string> lines = historyLines(game);
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

Move promptManualMove(const Game& game, PlayerMode playerOneMode, PlayerMode playerTwoMode) {
    while (true) {
        renderGame(
            game,
            playerOneMode,
            playerTwoMode,
            "Enter a move as a set or a bitmask number: ");

        std::string input;
        std::getline(std::cin, input);

        const std::optional<Move> move = parseMoveInput(input, game.E);
        if (!move.has_value()) {
            renderGame(game, playerOneMode, playerTwoMode, "", "Invalid input.");
            std::cout << "Press any key to continue.";
            _getch();
            continue;
        }

        if (!game.isMoveLegal(*move)) {
            renderGame(game, playerOneMode, playerTwoMode, "", "That move is illegal.");
            std::cout << "Press any key to continue.";
            _getch();
            continue;
        }

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

    renderGame(
        game,
        playerOneMode,
        playerTwoMode,
        "",
        ManipulateMove::moveLine(game.E, *move, static_cast<int>(game.size()) + 1, isPlayerOnesTurn) + ".");
    std::cout << "Press any key to continue.";
    _getch();

    return *move;
}
} // namespace

int main() {
    configureConsoleForUnicode();

    const PlayerMode playerOneMode = choosePlayerMode(1);
    const PlayerMode playerTwoMode = choosePlayerMode(2);
    const int n = promptUniversalSetSize();

    const Strategy playerOneStrategy = buildCustomStrategy1();
    const Strategy playerTwoStrategy = buildCustomStrategy2();

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
        else {
            const Strategy& strategy = isPlayerOnesTurn ? playerOneStrategy : playerTwoStrategy;
            move = chooseStrategyMove(strategy, game, playerOneMode, playerTwoMode, isPlayerOnesTurn);

            if (move == -1) {
                renderGame(game, playerOneMode, playerTwoMode);
                std::cout << "Player " << (isPlayerOnesTurn ? 2 : 1) << " wins.\n";
                std::cout << "Press any key to exit.";
                _getch();
                return 0;
            }
        }

        game.playMove(move);
    }
}
