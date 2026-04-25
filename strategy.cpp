#include "strategy.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <locale.h>
#include <mutex>
#include <sstream>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace {
void showMoveWhereErrorAndPause(const Game& position);
std::optional<std::string> findRuleNameForMoveImpl(const Strategy& strategy, const Game& position, Move move);

std::pair<std::size_t, std::size_t> rootSliceBounds(
    std::size_t total,
    std::size_t slice_index,
    std::size_t slice_count) {

    if (slice_count == 0) {
        slice_count = 1;
    }

    if (slice_index >= slice_count) {
        slice_index = slice_count - 1;
    }

    const std::size_t begin = (total * slice_index) / slice_count;
    const std::size_t end = (total * (slice_index + 1)) / slice_count;
    return { begin, end };
}
}

PickedOnMoveNode::PickedOnMoveNode(const IntExpr& move_number) : move_number(move_number) {}

bool PickedOnMoveNode::eval(const Game& game, Element element) const {
    const int moveNumber = ::eval(move_number, game);

    if (moveNumber < 1 || moveNumber > static_cast<int>(game.size())) {
        return false;
    }

    return ManipulateMove::hasElement(game[moveNumber - 1], element);
}

PickedInAnyNode::PickedInAnyNode(const MoveTest& test, PlayerFilter player_filter)
    : player_filter(player_filter), test(test) {}

bool PickedInAnyNode::eval(const Game& game, Element element) const {
    for (int i = 0; i < static_cast<int>(game.size()); i++) {
        const bool isMyMove = (i % 2) == (game.size() % 2);
        if (player_filter == Me && !isMyMove) {
            continue;
        }
        if (player_filter == Opponent && isMyMove) {
            continue;
        }

        const Move move = game[i];
        if (::eval(test, game, move) && ManipulateMove::hasElement(move, element)) {
            return true;
        }
    }

    return false;
}

TimesPickedCountNode::TimesPickedCountNode(const MoveTest& test) : test(test) {}

int TimesPickedCountNode::eval(const Game& game, Element element) const {
    int count = 0;

    for (Move move : game) {
        if (::eval(test, game, move) && ManipulateMove::hasElement(move, element)) {
            count++;
        }
    }

    return count;
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

bool FailElementNode::eval(const Game&, Element) const {
    return false;
}

bool PassElementNode::eval(const Game&, Element) const {
    return true;
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

AllThatNode::AllThatNode(const ElementTest& test) : test(test) {}

bool AllThatNode::eval(const Game& game, Move move) const {
    for (Element element = 0; element < game.E.size; element++) {
        if (::eval(test, game, element) && !ManipulateMove::hasElement(move, element)) {
            return false;
        }
    }

    return true;
}

OnlyFromNode::OnlyFromNode(const ElementTest& test) : test(test) {}

bool OnlyFromNode::eval(const Game& game, Move move) const {
    for (Element element = 0; element < game.E.size; element++) {
        if (ManipulateMove::hasElement(move, element) && !::eval(test, game, element)) {
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

AllButNode::AllButNode(const IntExpr& n, const ElementTest& test) : n(n), test(test) {}

bool AllButNode::eval(const Game& game, Move move) const {
    const int omitCount = ::eval(n, game);
    int total = 0;
    int picked = 0;

    for (Element element = 0; element < game.E.size; element++) {
        if (!::eval(test, game, element)) {
            continue;
        }

        total++;
        if (ManipulateMove::hasElement(move, element)) {
            picked++;
        }
    }

    return picked == total - omitCount;
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

SuchThatNode::SuchThatNode(const MoveTest& move_test, const Condition& condition)
    : move_test(move_test), condition(condition) {}

bool SuchThatNode::eval(const Game& game, Move move) const {
    if (!::eval(move_test, game, move)) {
        return false;
    }

    Game next = game;
    next.playMove(move);
    return ::eval(condition, next);
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

IsLegalNode::IsLegalNode(const MoveTest& test) : test(test) {}

bool IsLegalNode::eval(const Game& game) const {
    for (Move move : game.legalMoves()) {
        if (::eval(test, game, move)) {
            return true;
        }
    }

    return false;
}

QuantifiedMoveConditionNode::QuantifiedMoveConditionNode(
    const Condition& condition,
    const MoveTest& test,
    QuantifiedMoveConditionNode::Quantifier quantifier)
    : quantifier(quantifier), condition(condition), test(test) {}

bool QuantifiedMoveConditionNode::eval(const Game& game) const {
    bool matchedAny = false;

    for (Move move : game.legalMoves()) {
        if (!::eval(test, game, move)) {
            continue;
        }

        matchedAny = true;

        Game next = game;
        next.playMove(move);
        const bool conditionHolds = ::eval(condition, next);

        if (quantifier == QuantifiedMoveConditionNode::Ever && conditionHolds) {
            return true;
        }

        if (quantifier == QuantifiedMoveConditionNode::Always && !conditionHolds) {
            return false;
        }
    }

    return quantifier == QuantifiedMoveConditionNode::Always ? true : false;
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

MinElementIntNode::MinElementIntNode(const ElementIntExpr& expr, std::optional<ElementTest> domain)
    : expr(expr), domain(domain) {}

int MinElementIntNode::eval(const Game& game) const {
    int result = 0;
    bool found = false;

    for (Element element = 0; element < game.E.size; element++) {
        if (domain.has_value() && !::eval(*domain, game, element)) {
            continue;
        }

        const int value = ::eval(expr, game, element);
        if (!found || value < result) {
            result = value;
            found = true;
        }
    }

    return result;
}

MaxElementIntNode::MaxElementIntNode(const ElementIntExpr& expr, std::optional<ElementTest> domain)
    : expr(expr), domain(domain) {}

int MaxElementIntNode::eval(const Game& game) const {
    int result = 0;
    bool found = false;

    for (Element element = 0; element < game.E.size; element++) {
        if (domain.has_value() && !::eval(*domain, game, element)) {
            continue;
        }

        const int value = ::eval(expr, game, element);
        if (!found || value > result) {
            result = value;
            found = true;
        }
    }

    return result;
}

CompareElementIntNode::CompareElementIntNode(const ElementIntExpr& lhs, const IntExpr& rhs, Comparison op)
    : op(op), lhs(lhs), rhs(rhs) {}

bool CompareElementIntNode::eval(const Game& game, Element element) const {
    const int left = ::eval(lhs, game, element);
    const int right = ::eval(rhs, game);
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
    if (hasStrategyRuntimeError()) {
        return 0;
    }

    for (int i = 0; i < static_cast<int>(game.size()); i++) {
        if (::eval(test, game, game[i])) {
            return i + 1;
        }
    }

    showMoveWhereErrorAndPause(game);
    return 0;
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

ElementTest picked_in_any(const MoveTest& test) {
    return ElementTest{ std::make_shared<PickedInAnyNode>(test, PickedInAnyNode::AnyPlayer) };
}

ElementTest picked_by_me(const MoveTest& test) {
    return ElementTest{ std::make_shared<PickedInAnyNode>(test, PickedInAnyNode::Me) };
}

ElementTest picked_by_opponent(const MoveTest& test) {
    return ElementTest{ std::make_shared<PickedInAnyNode>(test, PickedInAnyNode::Opponent) };
}

const ElementTest fail = ElementTest{ std::make_shared<FailElementNode>() };
const ElementTest pass = ElementTest{ std::make_shared<PassElementNode>() };
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

MoveTest all_that(const ElementTest& test) {
    return MoveTest{ std::make_shared<AllThatNode>(test) };
}

MoveTest only_from(const ElementTest& test) {
    return MoveTest{ std::make_shared<OnlyFromNode>(test) };
}

MoveTest all_but(int n, const ElementTest& test) {
    return all_but(literal_int(n), test);
}

MoveTest all_but(const IntExpr& n, const ElementTest& test) {
    return MoveTest{ std::make_shared<AllButNode>(n, test) };
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
const Condition TRUE = Condition{ std::make_shared<TrueNode>() };
const Condition FALSE = Condition{ std::make_shared<FalseNode>() };

Condition true_condition() { return TRUE; }
Condition false_condition() { return FALSE; }

Condition there_is_an_element(const ElementTest& test) {
    return Condition{ std::make_shared<ExistsElementNode>(test) };
}

Condition has_been_played(const MoveTest& test) {
    return Condition{ std::make_shared<HasBeenPlayedNode>(test) };
}

Condition is_legal(const MoveTest& test) {
    return Condition{ std::make_shared<IsLegalNode>(test) };
}

Condition Condition::always_after(const MoveTest& test) const {
    return Condition{ std::make_shared<QuantifiedMoveConditionNode>(*this, test, QuantifiedMoveConditionNode::Always) };
}

Condition Condition::ever_after(const MoveTest& test) const {
    return Condition{ std::make_shared<QuantifiedMoveConditionNode>(*this, test, QuantifiedMoveConditionNode::Ever) };
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

ElementTest operator==(const ElementIntExpr& lhs, const IntExpr& rhs) {
    return ElementTest{ std::make_shared<CompareElementIntNode>(lhs, rhs, CompareElementIntNode::EQ) };
}

ElementTest operator==(const ElementIntExpr& lhs, int rhs) {
    return lhs == literal_int(rhs);
}

ElementTest operator==(int lhs, const ElementIntExpr& rhs) {
    return rhs == lhs;
}

ElementTest operator!=(const ElementIntExpr& lhs, const IntExpr& rhs) {
    return ElementTest{ std::make_shared<CompareElementIntNode>(lhs, rhs, CompareElementIntNode::NEQ) };
}

ElementTest operator!=(const ElementIntExpr& lhs, int rhs) {
    return lhs != literal_int(rhs);
}

ElementTest operator!=(int lhs, const ElementIntExpr& rhs) {
    return rhs != lhs;
}

IntExpr number_of_elements(const ElementTest& test) {
    return IntExpr{ std::make_shared<CountElementsNode>(test) };
}

IntExpr move_where(const MoveTest& test) {
    return IntExpr{ std::make_shared<MoveWhereNode>(test) };
}

IntExpr min(const ElementIntExpr& expr) {
    return IntExpr{ std::make_shared<MinElementIntNode>(expr) };
}

IntExpr min(const ElementIntExpr& expr, const ElementTest& domain) {
    return IntExpr{ std::make_shared<MinElementIntNode>(expr, domain) };
}

IntExpr max(const ElementIntExpr& expr) {
    return IntExpr{ std::make_shared<MaxElementIntNode>(expr) };
}

IntExpr max(const ElementIntExpr& expr, const ElementTest& domain) {
    return IntExpr{ std::make_shared<MaxElementIntNode>(expr, domain) };
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

MoveTest MoveTest::such_that(const Condition& cond) const {
    return MoveTest{ std::make_shared<SuchThatNode>(*this, cond) };
}

MoveTest::TimesPickedProxy::operator ElementIntExpr() const {
    return ElementIntExpr{ std::make_shared<TimesPickedCountNode>(*owner) };
}

void StrategyBuilder::flush_pending_ifs() {
    while (!if_stack.empty() && if_stack.back().awaiting_else) {
        current = if_stack.back().parent;
        if_stack.pop_back();
    }
}

void StrategyBuilder::flush_pending_ifs_above(int id) {
    while (!if_stack.empty() && if_stack.back().id != id && if_stack.back().awaiting_else) {
        current = if_stack.back().parent;
        if_stack.pop_back();
    }
}

void StrategyBuilder::pick(const MoveTest& m) {
    flush_pending_ifs();
    rules.push_back({ current, m, std::nullopt, std::nullopt });
}

void StrategyBuilder::pick(const MoveTest& m, const std::string& name) {
    flush_pending_ifs();
    rules.push_back({ current, m, name, std::nullopt });
}

void StrategyBuilder::throw_rule(const std::string& message) {
    flush_pending_ifs();
    rules.push_back({ current, nothing, std::nullopt, message });
}

Strategy StrategyBuilder::finish() const {
    StrategyBuilder copy = *this;
    copy.flush_pending_ifs();
    return Strategy{ copy.rules };
}

IfScope::IfScope(StrategyBuilder& builder, const Condition& cond)
    : builder(builder), id(builder.next_if_id++) {
    builder.flush_pending_ifs();
    builder.if_stack.push_back({ id, builder.current, cond, false });
    builder.current = builder.current && cond;
}

IfScope::~IfScope() {
    builder.flush_pending_ifs_above(id);
    if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
        builder.current = builder.if_stack.back().parent;
        builder.if_stack.back().awaiting_else = true;
    }
}

ElseScope::ElseScope(StrategyBuilder& builder)
    : builder(builder), id(builder.if_stack.back().id) {
    const IfFrame& frame = builder.if_stack.back();
    builder.current = frame.parent && !frame.cond;
    builder.if_stack.back().awaiting_else = false;
}

ElseScope::~ElseScope() {
    builder.flush_pending_ifs_above(id);
    if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
        builder.current = builder.if_stack.back().parent;
        builder.if_stack.pop_back();
    }
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

int eval(const ElementIntExpr& expr, const Game& game, Element element) {
    return expr.ptr->eval(game, element);
}

namespace {
thread_local std::string g_strategy_runtime_error;
thread_local const Strategy* g_active_strategy = nullptr;
thread_local bool g_active_strategy_is_player_one = false;
std::mutex g_strategy_output_mutex;
}

void clearStrategyRuntimeError() {
    g_strategy_runtime_error.clear();
}

bool hasStrategyRuntimeError() {
    return !g_strategy_runtime_error.empty();
}

std::string strategyRuntimeErrorMessage() {
    return g_strategy_runtime_error;
}

namespace {
void configureConsoleForUnicode() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
}

void logVerifierProgress(unsigned long long explored_positions) {
    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    std::cout << "Verifier explored " << explored_positions << " positions...\n";
}

void printHistory(const Game& game) {
    bool isPlayerOnesTurn = true;
    int moveNumber = 1;
    Game position{ game.E };

    std::cout << "History:\n";
    if (game.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (Move move : game) {
        const bool strategyMadeMove =
            g_active_strategy != nullptr
            && isPlayerOnesTurn == g_active_strategy_is_player_one;
        const std::optional<std::string> ruleName =
            strategyMadeMove ? findRuleNameForMoveImpl(*g_active_strategy, position, move) : std::nullopt;

        std::cout
            << "  "
            << ManipulateMove::moveLine(game.E, move, moveNumber, isPlayerOnesTurn, ruleName)
            << '\n';

        position.playMove(move);
        isPlayerOnesTurn = !isPlayerOnesTurn;
        moveNumber++;
    }
}

void showMoveWhereErrorAndPause(const Game& position) {
    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    configureConsoleForUnicode();
    std::cout << "move_where did not match any move in the game.\n\n";
    printHistory(position);
    g_strategy_runtime_error = "move_where: no move in the game passed the move test";
}

void showIllegalMoveErrorAndPause(const Game& position, Move move, const std::optional<std::string>& ruleName) {
    const bool isPlayerOnesTurn = (position.size() % 2 == 0);

    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    configureConsoleForUnicode();
    std::cout << "Strategy tried to play an illegal move.\n\n";
    printHistory(position);
    std::cout << "\nIllegal move:\n";
    std::cout
        << "  "
        << ManipulateMove::moveLine(
            position.E,
            move,
            static_cast<int>(position.size()) + 1,
            isPlayerOnesTurn,
            ruleName
        )
        << "\n\n";
    g_strategy_runtime_error = "illegal strategy move";
}

void showNoMatchingMoveErrorAndPause(const Game& position) {
    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    configureConsoleForUnicode();
    std::cout << "No move matched the strategy rules.\n\n";
    printHistory(position);
    g_strategy_runtime_error = "no move matched the strategy rules";
}

void showSuchThatNoMatchErrorAndPause(const Game& position, const std::optional<std::string>& ruleName) {
    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    configureConsoleForUnicode();
    std::cout << "such_that did not match any move that met the condition";
    if (ruleName.has_value()) {
        std::cout << " for rule \"" << *ruleName << "\"";
    }
    std::cout << ".\n\n";
    printHistory(position);
    g_strategy_runtime_error = "such_that did not match any move that met the condition";
}

void showCustomThrowErrorAndPause(const Game& position, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_strategy_output_mutex);
    configureConsoleForUnicode();
    std::cout << message << "\n\n";
    printHistory(position);
    g_strategy_runtime_error = message;
}

bool suchThatMatchedUnderlyingMovesButNoCondition(const MoveTest& test, const Game& position, const std::vector<Move>& candidates) {
    const auto suchThat = std::dynamic_pointer_cast<SuchThatNode>(test.ptr);
    if (!suchThat) {
        return false;
    }

    bool matchedUnderlying = false;
    for (Move candidate : candidates) {
        if (!::eval(suchThat->move_test, position, candidate)) {
            continue;
        }

        matchedUnderlying = true;

        Game next = position;
        next.playMove(candidate);
        if (::eval(suchThat->condition, next)) {
            return false;
        }
    }

    return matchedUnderlying;
}

void throwIfRuleAllowsIllegalMoves(const Rule& rule, const Game& position) {
    if (rule.throw_message.has_value()) {
        return;
    }

    for (Move candidate : position.allMoves()) {
        if (eval(rule.move, position, candidate) && !position.isMoveLegal(candidate)) {
            showIllegalMoveErrorAndPause(position, candidate, rule.name);
            return;
        }
    }
}

std::vector<Move> allowedFromCandidates(const Strategy& strategy, const Game& position, const std::vector<Move>& candidates) {
    if (hasStrategyRuntimeError()) {
        return {};
    }

    for (const Rule& rule : strategy.rules) {
        if (!eval(rule.guard, position)) {
            continue;
        }

        if (rule.throw_message.has_value()) {
            showCustomThrowErrorAndPause(position, *rule.throw_message);
            return {};
        }

        throwIfRuleAllowsIllegalMoves(rule, position);
        if (hasStrategyRuntimeError()) {
            return {};
        }

        std::vector<Move> result;
        for (Move candidate : candidates) {
            if (eval(rule.move, position, candidate)) {
                result.push_back(candidate);
            }
        }

        if (!result.empty()) {
            return result;
        }

        if (suchThatMatchedUnderlyingMovesButNoCondition(rule.move, position, candidates)) {
            showSuchThatNoMatchErrorAndPause(position, rule.name);
            return {};
        }
    }

    if (!candidates.empty()) {
        showNoMatchingMoveErrorAndPause(position);
    }

    return {};
}

std::optional<std::string> findRuleNameForMoveImpl(const Strategy& strategy, const Game& position, Move move) {
    const std::vector<Move> candidates = position.legalMoves();

    for (const Rule& rule : strategy.rules) {
        if (!eval(rule.guard, position)) {
            continue;
        }

        if (rule.throw_message.has_value()) {
            continue;
        }

        std::vector<Move> result;
        for (Move candidate : candidates) {
            if (eval(rule.move, position, candidate)) {
                result.push_back(candidate);
            }
        }

        if (!result.empty()) {
            for (Move candidate : result) {
                if (candidate == move) {
                    return rule.name;
                }
            }

            return std::nullopt;
        }
    }

    return std::nullopt;
}

StrategyVerificationResult verifyStrategyImpl(
    const Strategy& strategy,
    const Game& position,
    bool strategyPlayersTurn,
    unsigned long long& explored_positions,
    bool log_progress = true,
    std::atomic<unsigned long long>* progress_slot = nullptr,
    unsigned long long* pending_progress = nullptr) {

    if (hasStrategyRuntimeError()) {
        return {};
    }

    explored_positions++;
    if (log_progress && explored_positions % 100000 == 0) {
        logVerifierProgress(explored_positions);
    }
    if (progress_slot != nullptr && pending_progress != nullptr) {
        (*pending_progress)++;
        if (*pending_progress >= 4096) {
            progress_slot->fetch_add(*pending_progress, std::memory_order_relaxed);
            *pending_progress = 0;
        }
    }

    StrategyVerificationResult result;

    if (strategyPlayersTurn) {
        const std::vector<Move> moves = allowedLegalMoves(strategy, position);

        if (moves.empty()) {
            result.wins = false;
            return result;
        }

        for (Move move : moves) {
            Game next = position;
            next.playMove(move);

            StrategyVerificationResult child =
                verifyStrategyImpl(strategy, next, false, explored_positions, log_progress, progress_slot, pending_progress);
            if (hasStrategyRuntimeError()) {
                return {};
            }
            if (!child.wins) {
                result.wins = false;
                result.line.push_back(move);
                result.line.insert(result.line.end(), child.line.begin(), child.line.end());
                return result;
            }
        }

        result.wins = true;
        return result;
    }

    const std::vector<Move> replies = position.legalMoves();
    if (replies.empty()) {
        result.wins = true;
        return result;
    }

    for (Move reply : replies) {
        Game next = position;
        next.playMove(reply);

        StrategyVerificationResult child =
            verifyStrategyImpl(strategy, next, true, explored_positions, log_progress, progress_slot, pending_progress);
        if (hasStrategyRuntimeError()) {
            return {};
        }
        if (!child.wins) {
            result.wins = false;
            result.line.push_back(reply);
            result.line.insert(result.line.end(), child.line.begin(), child.line.end());
            return result;
        }
    }

    result.wins = true;
    return result;
}

struct ParallelBranchResult {
    StrategyVerificationResult result;
    std::string runtime_error;
    unsigned long long explored_positions = 0;
};

ParallelBranchResult verifyStrategyBranch(
    const Strategy& strategy,
    const Game& position,
    bool strategyPlayersTurn,
    bool strategyIsPlayerOne,
    std::atomic<unsigned long long>* progress_slot) {

    clearStrategyRuntimeError();
    g_active_strategy = &strategy;
    g_active_strategy_is_player_one = strategyIsPlayerOne;

    unsigned long long explored_positions = 0;
    unsigned long long pending_progress = 0;
    const StrategyVerificationResult result =
        verifyStrategyImpl(strategy, position, strategyPlayersTurn, explored_positions, false, progress_slot, &pending_progress);
    if (progress_slot != nullptr && pending_progress != 0) {
        progress_slot->fetch_add(pending_progress, std::memory_order_relaxed);
    }
    const std::string runtime_error = strategyRuntimeErrorMessage();

    g_active_strategy = nullptr;
    clearStrategyRuntimeError();
    return { result, runtime_error, explored_positions };
}
} // namespace

std::optional<std::string> ruleNameForMove(const Strategy& strategy, const Game& position, Move move) {
    return findRuleNameForMoveImpl(strategy, position, move);
}

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
    clearStrategyRuntimeError();
    g_active_strategy = &strategy;
    g_active_strategy_is_player_one = strategyPlayersTurn;
    unsigned long long explored_positions = 0;
    const StrategyVerificationResult result =
        verifyStrategyImpl(strategy, position, strategyPlayersTurn, explored_positions);
    g_active_strategy = nullptr;
    return result;
}

StrategyVerificationResult verifyStrategyParallel(const Strategy& strategy, const Game& position, bool strategyPlayersTurn) {
    return verifyStrategyParallelSlice(strategy, position, strategyPlayersTurn, 0, 1);
}

StrategyVerificationResult verifyStrategyParallelSlice(
    const Strategy& strategy,
    const Game& position,
    bool strategyPlayersTurn,
    std::size_t slice_index,
    std::size_t slice_count) {

    const unsigned int hardware_threads = std::thread::hardware_concurrency();
    if (hardware_threads < 2 && slice_count == 1) {
        return verifyStrategy(strategy, position, strategyPlayersTurn);
    }

    clearStrategyRuntimeError();
    g_active_strategy = &strategy;
    g_active_strategy_is_player_one = strategyPlayersTurn;

    if (strategyPlayersTurn) {
        const std::vector<Move> all_moves = allowedLegalMoves(strategy, position);
        if (hasStrategyRuntimeError()) {
            g_active_strategy = nullptr;
            return {};
        }

        if (all_moves.empty()) {
            g_active_strategy = nullptr;
            return {};
        }

        if (slice_count == 1 && all_moves.size() < 2) {
            g_active_strategy = nullptr;
            return verifyStrategy(strategy, position, strategyPlayersTurn);
        }

        const auto [begin, end] = rootSliceBounds(all_moves.size(), slice_index, slice_count);
        if (begin >= end) {
            g_active_strategy = nullptr;
            return {};
        }

        std::vector<Move> moves(all_moves.begin() + static_cast<std::ptrdiff_t>(begin), all_moves.begin() + static_cast<std::ptrdiff_t>(end));

        if (moves.size() == 1) {
            Game next = position;
            next.playMove(moves[0]);
            ParallelBranchResult child = verifyStrategyBranch(strategy, next, false, strategyPlayersTurn, nullptr);
            if (!child.runtime_error.empty()) {
                g_strategy_runtime_error = child.runtime_error;
                g_active_strategy = nullptr;
                return {};
            }

            StrategyVerificationResult result;
            result.wins = child.result.wins;
            result.line.push_back(moves[0]);
            result.line.insert(result.line.end(), child.result.line.begin(), child.result.line.end());
            g_active_strategy = nullptr;
            return result;
        }

        std::vector<std::atomic<unsigned long long>> branch_progress(moves.size());
        std::vector<std::future<ParallelBranchResult>> futures;
        futures.reserve(moves.size());

        for (int i = 0; i < static_cast<int>(moves.size()); i++) {
            branch_progress[i].store(0, std::memory_order_relaxed);
            const Move move = moves[i];
            futures.push_back(std::async(std::launch::async, [&strategy, position, move, strategyPlayersTurn, &branch_progress, i]() {
                Game next = position;
                next.playMove(move);
                return verifyStrategyBranch(strategy, next, false, strategyPlayersTurn, &branch_progress[i]);
            }));
        }

        std::atomic<bool> stop_progress{ false };
        std::thread progress_thread([&branch_progress, &stop_progress]() {
            unsigned long long next_report = 100000;
            while (!stop_progress.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                unsigned long long total_explored_positions = 0;
                for (auto& counter : branch_progress) {
                    total_explored_positions += counter.load(std::memory_order_relaxed);
                }

                while (total_explored_positions >= next_report) {
                    logVerifierProgress(next_report);
                    next_report += 100000;
                }
            }
        });

        StrategyVerificationResult result;
        for (int i = 0; i < static_cast<int>(moves.size()); i++) {
            ParallelBranchResult child = futures[i].get();
            if (!child.runtime_error.empty()) {
                stop_progress.store(true, std::memory_order_relaxed);
                progress_thread.join();
                g_strategy_runtime_error = child.runtime_error;
                g_active_strategy = nullptr;
                return {};
            }

            if (!child.result.wins) {
                result.wins = false;
                result.line.push_back(moves[i]);
                result.line.insert(result.line.end(), child.result.line.begin(), child.result.line.end());
                stop_progress.store(true, std::memory_order_relaxed);
                progress_thread.join();
                g_active_strategy = nullptr;
                return result;
            }
        }

        result.wins = true;
        stop_progress.store(true, std::memory_order_relaxed);
        progress_thread.join();
        g_active_strategy = nullptr;
        return result;
    }

    const std::vector<Move> all_replies = position.legalMoves();
    if (all_replies.empty()) {
        g_active_strategy = nullptr;
        return StrategyVerificationResult{ true, {} };
    }

    if (slice_count == 1 && all_replies.size() < 2) {
        g_active_strategy = nullptr;
        return verifyStrategy(strategy, position, strategyPlayersTurn);
    }

    const auto [begin, end] = rootSliceBounds(all_replies.size(), slice_index, slice_count);
    if (begin >= end) {
        g_active_strategy = nullptr;
        return StrategyVerificationResult{ true, {} };
    }

    std::vector<Move> replies(all_replies.begin() + static_cast<std::ptrdiff_t>(begin), all_replies.begin() + static_cast<std::ptrdiff_t>(end));

    if (replies.size() == 1) {
        Game next = position;
        next.playMove(replies[0]);
        ParallelBranchResult child = verifyStrategyBranch(strategy, next, true, strategyPlayersTurn, nullptr);
        if (!child.runtime_error.empty()) {
            g_strategy_runtime_error = child.runtime_error;
            g_active_strategy = nullptr;
            return {};
        }

        StrategyVerificationResult result;
        result.wins = child.result.wins;
        if (!child.result.wins) {
            result.line.push_back(replies[0]);
            result.line.insert(result.line.end(), child.result.line.begin(), child.result.line.end());
        }

        g_active_strategy = nullptr;
        return result;
    }

    std::vector<std::atomic<unsigned long long>> branch_progress(replies.size());
    std::vector<std::future<ParallelBranchResult>> futures;
    futures.reserve(replies.size());

    for (int i = 0; i < static_cast<int>(replies.size()); i++) {
        branch_progress[i].store(0, std::memory_order_relaxed);
        const Move reply = replies[i];
        futures.push_back(std::async(std::launch::async, [&strategy, position, reply, strategyPlayersTurn, &branch_progress, i]() {
            Game next = position;
            next.playMove(reply);
            return verifyStrategyBranch(strategy, next, true, strategyPlayersTurn, &branch_progress[i]);
        }));
    }

    std::atomic<bool> stop_progress{ false };
    std::thread progress_thread([&branch_progress, &stop_progress]() {
        unsigned long long next_report = 100000;
        while (!stop_progress.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            unsigned long long total_explored_positions = 0;
            for (auto& counter : branch_progress) {
                total_explored_positions += counter.load(std::memory_order_relaxed);
            }

            while (total_explored_positions >= next_report) {
                logVerifierProgress(next_report);
                next_report += 100000;
            }
        }
    });

    StrategyVerificationResult result;
    for (int i = 0; i < static_cast<int>(replies.size()); i++) {
        ParallelBranchResult child = futures[i].get();
        if (!child.runtime_error.empty()) {
            stop_progress.store(true, std::memory_order_relaxed);
            progress_thread.join();
            g_strategy_runtime_error = child.runtime_error;
            g_active_strategy = nullptr;
            return {};
        }

        if (!child.result.wins) {
            result.wins = false;
            result.line.push_back(replies[i]);
            result.line.insert(result.line.end(), child.result.line.begin(), child.result.line.end());
            stop_progress.store(true, std::memory_order_relaxed);
            progress_thread.join();
            g_active_strategy = nullptr;
            return result;
        }
    }

    result.wins = true;
    stop_progress.store(true, std::memory_order_relaxed);
    progress_thread.join();
    g_active_strategy = nullptr;
    return result;
}
