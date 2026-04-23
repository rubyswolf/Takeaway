#include "strategy.h"

#include <stdexcept>
#include <sstream>
#include <unordered_map>

PickedOnMoveNode::PickedOnMoveNode(const IntExpr& move_number) : move_number(move_number) {}

bool PickedOnMoveNode::eval(const Game& game, Element element) const {
    const int moveNumber = ::eval(move_number, game);

    if (moveNumber < 1 || moveNumber > static_cast<int>(game.size())) {
        return false;
    }

    return ManipulateMove::hasElement(game[moveNumber - 1], element);
}

bool IsSingletonNode::eval(const Game& game, Element element) const {
    const Move singletonMove = 1 << element;

    for (Move move : game) {
        if (move == singletonMove) {
            return true;
        }
    }

    return false;
}

bool AnythingNode::eval(const Game& game, Move move) const {
    return game.isMoveLegal(move);
}

bool NothingNode::eval(const Game&, Move) const {
    return false;
}

bool EverythingNode::eval(const Game& game, Move move) const {
    return move == game.E.bitmask;
}

AllElementsNode::AllElementsNode(const ElementTest& test) : test(test) {}

bool AllElementsNode::eval(const Game& game, Move move) const {
    for (Element element = 0; element < game.E.size; element++) {
        const bool picked = ManipulateMove::hasElement(move, element);
        const bool passes = ::eval(test, game, element);
        if (picked != passes) {
            return false;
        }
    }

    return true;
}

AnyFromNode::AnyFromNode(const IntExpr& n, const ElementTest& test) : n(n), test(test) {}

bool AnyFromNode::eval(const Game& game, Move move) const {
    const int target = ::eval(n, game);
    int count = 0;

    for (Element element = 0; element < game.E.size; element++) {
        if (ManipulateMove::hasElement(move, element) && ::eval(test, game, element)) {
            count++;
        }
    }

    return count == target;
}

NotElementTestNode::NotElementTestNode(const ElementTest& inner) : inner(inner) {}

bool NotElementTestNode::eval(const Game& game, Element element) const {
    return !::eval(inner, game, element);
}

NotMoveTestNode::NotMoveTestNode(const MoveTest& inner) : inner(inner) {}

bool NotMoveTestNode::eval(const Game& game, Move move) const {
    return !::eval(inner, game, move);
}

AndElementTestNode::AndElementTestNode(const ElementTest& a, const ElementTest& b) : a(a), b(b) {}

bool AndElementTestNode::eval(const Game& game, Element element) const {
    return ::eval(a, game, element) && ::eval(b, game, element);
}

AndMoveTestNode::AndMoveTestNode(const MoveTest& a, const MoveTest& b) : a(a), b(b) {}

bool AndMoveTestNode::eval(const Game& game, Move move) const {
    return ::eval(a, game, move) && ::eval(b, game, move);
}

OrElementTestNode::OrElementTestNode(const ElementTest& a, const ElementTest& b) : a(a), b(b) {}

bool OrElementTestNode::eval(const Game& game, Element element) const {
    return ::eval(a, game, element) || ::eval(b, game, element);
}

OrMoveTestNode::OrMoveTestNode(const MoveTest& a, const MoveTest& b) : a(a), b(b) {}

bool OrMoveTestNode::eval(const Game& game, Move move) const {
    return ::eval(a, game, move) || ::eval(b, game, move);
}

SelectElementTestNode::SelectElementTestNode(const Condition& cond, const ElementTest& a, const ElementTest& b)
    : cond(cond), a(a), b(b) {}

bool SelectElementTestNode::eval(const Game& game, Element element) const {
    return ::eval(cond, game) ? ::eval(a, game, element) : ::eval(b, game, element);
}

SelectMoveTestNode::SelectMoveTestNode(const Condition& cond, const MoveTest& a, const MoveTest& b)
    : cond(cond), a(a), b(b) {}

bool SelectMoveTestNode::eval(const Game& game, Move move) const {
    return ::eval(cond, game) ? ::eval(a, game, move) : ::eval(b, game, move);
}

bool TrueNode::eval(const Game&) const {
    return true;
}

bool FalseNode::eval(const Game&) const {
    return false;
}

ExistsElementNode::ExistsElementNode(const ElementTest& test) : test(test) {}

bool ExistsElementNode::eval(const Game& game) const {
    for (Element element = 0; element < game.E.size; element++) {
        if (::eval(test, game, element)) {
            return true;
        }
    }

    return false;
}

HasBeenPlayedNode::HasBeenPlayedNode(const MoveTest& test) : test(test) {}

bool HasBeenPlayedNode::eval(const Game& game) const {
    for (Move move : game) {
        if (::eval(test, game, move)) {
            return true;
        }
    }

    return false;
}

CompareIntNode::CompareIntNode(const IntExpr& lhs, const IntExpr& rhs, Comparison op)
    : op(op), lhs(lhs), rhs(rhs) {}

bool CompareIntNode::eval(const Game& game) const {
    const int left = ::eval(lhs, game);
    const int right = ::eval(rhs, game);
    return op == EQ ? left == right : left != right;
}

NotConditionNode::NotConditionNode(const Condition& inner) : inner(inner) {}

bool NotConditionNode::eval(const Game& game) const {
    return !::eval(inner, game);
}

AndConditionNode::AndConditionNode(const Condition& a, const Condition& b) : a(a), b(b) {}

bool AndConditionNode::eval(const Game& game) const {
    return ::eval(a, game) && ::eval(b, game);
}

OrConditionNode::OrConditionNode(const Condition& a, const Condition& b) : a(a), b(b) {}

bool OrConditionNode::eval(const Game& game) const {
    return ::eval(a, game) || ::eval(b, game);
}

CompareConditionsNode::CompareConditionsNode(const Condition& lhs, const Condition& rhs, Comparison op)
    : op(op), lhs(lhs), rhs(rhs) {}

bool CompareConditionsNode::eval(const Game& game) const {
    const bool left = ::eval(lhs, game);
    const bool right = ::eval(rhs, game);
    return op == EQ ? left == right : left != right;
}

LiteralIntNode::LiteralIntNode(int value) : value(value) {}

int LiteralIntNode::eval(const Game&) const {
    return value;
}

int CurrentMoveNode::eval(const Game& game) const {
    return static_cast<int>(game.size()) + 1;
}

int PreviousMoveNode::eval(const Game& game) const {
    return static_cast<int>(game.size());
}

CountElementsNode::CountElementsNode(const ElementTest& test) : test(test) {}

int CountElementsNode::eval(const Game& game) const {
    int count = 0;

    for (Element element = 0; element < game.E.size; element++) {
        if (::eval(test, game, element)) {
            count++;
        }
    }

    return count;
}

MoveWhereNode::MoveWhereNode(const MoveTest& test) : test(test) {}

int MoveWhereNode::eval(const Game& game) const {
    for (int i = 0; i < static_cast<int>(game.size()); i++) {
        if (::eval(test, game, game[i])) {
            return i + 1;
        }
    }

    throw std::runtime_error("move_where: no move in the game passed the move test");
}

AddIntNode::AddIntNode(const IntExpr& lhs, const IntExpr& rhs) : lhs(lhs), rhs(rhs) {}

int AddIntNode::eval(const Game& game) const {
    return ::eval(lhs, game) + ::eval(rhs, game);
}

SubtractIntNode::SubtractIntNode(const IntExpr& lhs, const IntExpr& rhs) : lhs(lhs), rhs(rhs) {}

int SubtractIntNode::eval(const Game& game) const {
    return ::eval(lhs, game) - ::eval(rhs, game);
}

namespace {
IntExpr literal_int(int value) {
    return IntExpr{ std::make_shared<LiteralIntNode>(value) };
}
} // namespace

ElementTest picked_on_move(int move_number) {
    return picked_on_move(literal_int(move_number));
}

ElementTest picked_on_move(const IntExpr& move_number) {
    return ElementTest{ std::make_shared<PickedOnMoveNode>(move_number) };
}

const ElementTest is_singleton = ElementTest{ std::make_shared<IsSingletonNode>() };
const ElementTest are_singleton = is_singleton;

ElementTest operator~(const ElementTest& inner) {
    return ElementTest{ std::make_shared<NotElementTestNode>(inner) };
}

ElementTest operator&(const ElementTest& a, const ElementTest& b) {
    return ElementTest{ std::make_shared<AndElementTestNode>(a, b) };
}

ElementTest operator|(const ElementTest& a, const ElementTest& b) {
    return ElementTest{ std::make_shared<OrElementTestNode>(a, b) };
}

MoveTest all_elements(const ElementTest& test) {
    return MoveTest{ std::make_shared<AllElementsNode>(test) };
}

MoveTest any_from(int n, const ElementTest& test) {
    return any_from(literal_int(n), test);
}

MoveTest any_from(const IntExpr& n, const ElementTest& test) {
    return MoveTest{ std::make_shared<AnyFromNode>(n, test) };
}

MoveTest operator~(const MoveTest& inner) {
    return MoveTest{ std::make_shared<NotMoveTestNode>(inner) };
}

MoveTest operator&(const MoveTest& a, const MoveTest& b) {
    return MoveTest{ std::make_shared<AndMoveTestNode>(a, b) };
}

MoveTest operator|(const MoveTest& a, const MoveTest& b) {
    return MoveTest{ std::make_shared<OrMoveTestNode>(a, b) };
}

const MoveTest anything = MoveTest{ std::make_shared<AnythingNode>() };
const MoveTest nothing = MoveTest{ std::make_shared<NothingNode>() };
const MoveTest everything = MoveTest{ std::make_shared<EverythingNode>() };

Condition true_condition() {
    return Condition{ std::make_shared<TrueNode>() };
}

Condition false_condition() {
    return Condition{ std::make_shared<FalseNode>() };
}

Condition there_is_an_element(const ElementTest& test) {
    return Condition{ std::make_shared<ExistsElementNode>(test) };
}

Condition has_been_played(const MoveTest& test) {
    return Condition{ std::make_shared<HasBeenPlayedNode>(test) };
}

Condition operator!(const Condition& inner) {
    return Condition{ std::make_shared<NotConditionNode>(inner) };
}

Condition operator&&(const Condition& a, const Condition& b) {
    return Condition{ std::make_shared<AndConditionNode>(a, b) };
}

Condition operator||(const Condition& a, const Condition& b) {
    return Condition{ std::make_shared<OrConditionNode>(a, b) };
}

Condition operator==(const IntExpr& lhs, const IntExpr& rhs) {
    return Condition{ std::make_shared<CompareIntNode>(lhs, rhs, CompareIntNode::EQ) };
}

Condition operator==(const IntExpr& lhs, int rhs) {
    return lhs == literal_int(rhs);
}

Condition operator==(int lhs, const IntExpr& rhs) {
    return literal_int(lhs) == rhs;
}

Condition operator!=(const IntExpr& lhs, const IntExpr& rhs) {
    return Condition{ std::make_shared<CompareIntNode>(lhs, rhs, CompareIntNode::NEQ) };
}

Condition operator!=(const IntExpr& lhs, int rhs) {
    return lhs != literal_int(rhs);
}

Condition operator!=(int lhs, const IntExpr& rhs) {
    return literal_int(lhs) != rhs;
}

Condition operator==(const Condition& lhs, const Condition& rhs) {
    return Condition{ std::make_shared<CompareConditionsNode>(lhs, rhs, CompareConditionsNode::EQ) };
}

Condition operator!=(const Condition& lhs, const Condition& rhs) {
    return Condition{ std::make_shared<CompareConditionsNode>(lhs, rhs, CompareConditionsNode::NEQ) };
}

IntExpr number_of_elements(const ElementTest& test) {
    return IntExpr{ std::make_shared<CountElementsNode>(test) };
}

IntExpr move_where(const MoveTest& test) {
    return IntExpr{ std::make_shared<MoveWhereNode>(test) };
}

IntExpr operator+(const IntExpr& lhs, const IntExpr& rhs) {
    return IntExpr{ std::make_shared<AddIntNode>(lhs, rhs) };
}

IntExpr operator+(const IntExpr& lhs, int rhs) {
    return lhs + literal_int(rhs);
}

IntExpr operator+(int lhs, const IntExpr& rhs) {
    return literal_int(lhs) + rhs;
}

IntExpr operator-(const IntExpr& lhs, const IntExpr& rhs) {
    return IntExpr{ std::make_shared<SubtractIntNode>(lhs, rhs) };
}

IntExpr operator-(const IntExpr& lhs, int rhs) {
    return lhs - literal_int(rhs);
}

IntExpr operator-(int lhs, const IntExpr& rhs) {
    return literal_int(lhs) - rhs;
}

const IntExpr current_move = IntExpr{ std::make_shared<CurrentMoveNode>() };
const IntExpr previous_move = IntExpr{ std::make_shared<PreviousMoveNode>() };

ElementTest ElementTestWhenBuilder::otherwise(const ElementTest& b) const {
    return ElementTest{ std::make_shared<SelectElementTestNode>(cond, a, b) };
}

MoveTest MoveTestWhenBuilder::otherwise(const MoveTest& b) const {
    return MoveTest{ std::make_shared<SelectMoveTestNode>(cond, a, b) };
}

ElementTestWhenBuilder ElementTest::when(const Condition& cond) const {
    return ElementTestWhenBuilder{ *this, cond };
}

MoveTestWhenBuilder MoveTest::when(const Condition& cond) const {
    return MoveTestWhenBuilder{ *this, cond };
}

void StrategyBuilder::pick(const MoveTest& m) {
    rules.push_back({ current, m, while_legal_depth > 0 });
}

Strategy StrategyBuilder::finish() const {
    return Strategy{ rules };
}

IfScope::IfScope(StrategyBuilder& builder, const Condition& cond)
    : builder(builder), id(builder.next_if_id++) {
    builder.if_stack.push_back({ id, builder.current, cond });
    builder.current = builder.current && cond;
}

IfScope::~IfScope() {
    if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
        builder.current = builder.if_stack.back().parent;
        builder.if_stack.pop_back();
    }
}

ElseScope::ElseScope(StrategyBuilder& builder)
    : builder(builder), id(builder.if_stack.back().id) {
    const IfFrame& frame = builder.if_stack.back();
    builder.current = frame.parent && !frame.cond;
}

ElseScope::~ElseScope() {
    if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
        builder.current = builder.if_stack.back().parent;
        builder.if_stack.pop_back();
    }
}

WhileLegalScope::WhileLegalScope(StrategyBuilder& builder) : builder(builder) {
    builder.while_legal_depth++;
}

WhileLegalScope::~WhileLegalScope() {
    builder.while_legal_depth--;
}

bool eval(const ElementTest& test, const Game& game, Element element) {
    return test.ptr->eval(game, element);
}

bool eval(const MoveTest& test, const Game& game, Move move) {
    return test.ptr->eval(game, move);
}

bool eval(const Condition& condition, const Game& game) {
    return condition.ptr->eval(game);
}

int eval(const IntExpr& expr, const Game& game) {
    return expr.ptr->eval(game);
}

namespace {
void throwIfRuleAllowsIllegalMoves(const Rule& rule, const Game& position) {
    if (rule.allow_illegal) {
        return;
    }

    for (Move candidate : position.allMoves()) {
        if (eval(rule.move, position, candidate) && !position.isMoveLegal(candidate)) {
            throw std::runtime_error("pick: rule matched an illegal move outside WHILE_LEGAL");
        }
    }
}

std::vector<Move> allowedFromCandidates(const Strategy& strategy, const Game& position, const std::vector<Move>& candidates) {
    for (const Rule& rule : strategy.rules) {
        if (!eval(rule.guard, position)) {
            continue;
        }

        throwIfRuleAllowsIllegalMoves(rule, position);

        std::vector<Move> result;
        for (Move candidate : candidates) {
            if (eval(rule.move, position, candidate)) {
                result.push_back(candidate);
            }
        }

        if (!result.empty()) {
            return result;
        }
    }

    return {};
}

std::string makeStateKey(const Game& position, bool strategyPlayersTurn) {
    std::ostringstream out;
    out << position.E.size << '|' << (strategyPlayersTurn ? 'S' : 'O');

    for (Move move : position) {
        out << '|' << move;
    }

    return out.str();
}

StrategyVerificationResult verifyStrategyImpl(
    const Strategy& strategy,
    const Game& position,
    bool strategyPlayersTurn,
    std::unordered_map<std::string, StrategyVerificationResult>& memo) {

    const std::string key = makeStateKey(position, strategyPlayersTurn);
    const auto found = memo.find(key);
    if (found != memo.end()) {
        return found->second;
    }

    StrategyVerificationResult result;

    if (strategyPlayersTurn) {
        const std::vector<Move> moves = allowedLegalMoves(strategy, position);

        if (moves.empty()) {
            result.wins = false;
            memo[key] = result;
            return result;
        }

        for (Move move : moves) {
            Game next = position;
            next.playMove(move);

            StrategyVerificationResult child = verifyStrategyImpl(strategy, next, false, memo);
            if (child.wins) {
                result.wins = true;
                result.line.push_back(move);
                result.line.insert(result.line.end(), child.line.begin(), child.line.end());
                memo[key] = result;
                return result;
            }

            if (result.line.empty()) {
                result.line.push_back(move);
                result.line.insert(result.line.end(), child.line.begin(), child.line.end());
            }
        }

        result.wins = false;
        memo[key] = result;
        return result;
    }

    const std::vector<Move> replies = position.legalMoves();
    if (replies.empty()) {
        result.wins = true;
        memo[key] = result;
        return result;
    }

    for (Move reply : replies) {
        Game next = position;
        next.playMove(reply);

        StrategyVerificationResult child = verifyStrategyImpl(strategy, next, true, memo);
        if (!child.wins) {
            result.wins = false;
            result.line.push_back(reply);
            result.line.insert(result.line.end(), child.line.begin(), child.line.end());
            memo[key] = result;
            return result;
        }
    }

    result.wins = true;
    memo[key] = result;
    return result;
}
} // namespace

std::vector<Move> allowedMoves(const Strategy& strategy, const Game& position) {
    return allowedFromCandidates(strategy, position, position.allMoves());
}

std::vector<Move> allowedLegalMoves(const Strategy& strategy, const Game& position) {
    return allowedFromCandidates(strategy, position, position.legalMoves());
}

std::vector<Move> allowedPrincipalLegalMoves(const Strategy& strategy, const Game& position) {
    return allowedFromCandidates(strategy, position, position.principalLegalMoves());
}

std::optional<Move> firstAllowedLegalMove(const Strategy& strategy, const Game& position) {
    const std::vector<Move> moves = allowedLegalMoves(strategy, position);
    if (moves.empty()) {
        return std::nullopt;
    }

    return moves.front();
}

void printAllowedMoves(const Strategy& strategy, const Game& position, std::ostream& out) {
    for (Move move : allowedMoves(strategy, position)) {
        out << ManipulateMove::toString(position.E, move) << '\n';
    }
}

bool strategyWinsFrom(const Strategy& strategy, const Game& position, bool strategyPlayersTurn) {
    return verifyStrategy(strategy, position, strategyPlayersTurn).wins;
}

StrategyVerificationResult verifyStrategy(const Strategy& strategy, const Game& position, bool strategyPlayersTurn) {
    std::unordered_map<std::string, StrategyVerificationResult> memo;
    return verifyStrategyImpl(strategy, position, strategyPlayersTurn, memo);
}
