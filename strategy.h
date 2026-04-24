#pragma once

#include "takeaway.h"

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

template<typename Node>
using Ptr = std::shared_ptr<Node>;

struct ElementTestNode;
struct MoveTestNode;
struct ConditionNode;
struct IntNode;

struct ElementTest;
struct MoveTest;
struct Condition;
struct IntExpr;

struct ElementTestWhenBuilder;
struct MoveTestWhenBuilder;

struct ElementTest {
    Ptr<ElementTestNode> ptr;

    ElementTestWhenBuilder when(const Condition& cond) const;
};

struct MoveTest {
    Ptr<MoveTestNode> ptr;

    MoveTestWhenBuilder when(const Condition& cond) const;
};

struct Condition {
    Ptr<ConditionNode> ptr;
};

struct IntExpr {
    Ptr<IntNode> ptr;
};

bool eval(const ElementTest& test, const Game& game, Element element);
bool eval(const MoveTest& test, const Game& game, Move move);
bool eval(const Condition& condition, const Game& game);
int eval(const IntExpr& expr, const Game& game);

struct ElementTestNode {
    virtual ~ElementTestNode() = default;
    virtual bool eval(const Game& game, Element element) const = 0;
};

struct MoveTestNode {
    virtual ~MoveTestNode() = default;
    virtual bool eval(const Game& game, Move move) const = 0;
};

struct ConditionNode {
    virtual ~ConditionNode() = default;
    virtual bool eval(const Game& game) const = 0;
};

struct IntNode {
    virtual ~IntNode() = default;
    virtual int eval(const Game& game) const = 0;
};

struct PickedOnMoveNode : ElementTestNode {
    IntExpr move_number;

    explicit PickedOnMoveNode(const IntExpr& move_number);
    bool eval(const Game& game, Element element) const override;
};

struct IsSingletonNode : ElementTestNode {
    bool eval(const Game& game, Element element) const override;
};

struct FailElementNode : ElementTestNode {
    bool eval(const Game& game, Element element) const override;
};

struct AnythingNode : MoveTestNode {
    bool eval(const Game& game, Move move) const override;
};

struct NothingNode : MoveTestNode {
    bool eval(const Game& game, Move move) const override;
};

struct EverythingNode : MoveTestNode {
    bool eval(const Game& game, Move move) const override;
};

struct AllElementsNode : MoveTestNode {
    ElementTest test;

    explicit AllElementsNode(const ElementTest& test);
    bool eval(const Game& game, Move move) const override;
};

struct AllThatNode : MoveTestNode {
    ElementTest test;

    explicit AllThatNode(const ElementTest& test);
    bool eval(const Game& game, Move move) const override;
};

struct AnyFromNode : MoveTestNode {
    IntExpr n;
    ElementTest test;

    AnyFromNode(const IntExpr& n, const ElementTest& test);
    bool eval(const Game& game, Move move) const override;
};

struct AllButNode : MoveTestNode {
    IntExpr n;
    ElementTest test;

    AllButNode(const IntExpr& n, const ElementTest& test);
    bool eval(const Game& game, Move move) const override;
};

struct NotElementTestNode : ElementTestNode {
    ElementTest inner;

    explicit NotElementTestNode(const ElementTest& inner);
    bool eval(const Game& game, Element element) const override;
};

struct NotMoveTestNode : MoveTestNode {
    MoveTest inner;

    explicit NotMoveTestNode(const MoveTest& inner);
    bool eval(const Game& game, Move move) const override;
};

struct AndElementTestNode : ElementTestNode {
    ElementTest a;
    ElementTest b;

    AndElementTestNode(const ElementTest& a, const ElementTest& b);
    bool eval(const Game& game, Element element) const override;
};

struct AndMoveTestNode : MoveTestNode {
    MoveTest a;
    MoveTest b;

    AndMoveTestNode(const MoveTest& a, const MoveTest& b);
    bool eval(const Game& game, Move move) const override;
};

struct OrElementTestNode : ElementTestNode {
    ElementTest a;
    ElementTest b;

    OrElementTestNode(const ElementTest& a, const ElementTest& b);
    bool eval(const Game& game, Element element) const override;
};

struct OrMoveTestNode : MoveTestNode {
    MoveTest a;
    MoveTest b;

    OrMoveTestNode(const MoveTest& a, const MoveTest& b);
    bool eval(const Game& game, Move move) const override;
};

struct SelectElementTestNode : ElementTestNode {
    Condition cond;
    ElementTest a;
    ElementTest b;

    SelectElementTestNode(const Condition& cond, const ElementTest& a, const ElementTest& b);
    bool eval(const Game& game, Element element) const override;
};

struct SelectMoveTestNode : MoveTestNode {
    Condition cond;
    MoveTest a;
    MoveTest b;

    SelectMoveTestNode(const Condition& cond, const MoveTest& a, const MoveTest& b);
    bool eval(const Game& game, Move move) const override;
};

struct TrueNode : ConditionNode {
    bool eval(const Game& game) const override;
};

struct FalseNode : ConditionNode {
    bool eval(const Game& game) const override;
};

struct ExistsElementNode : ConditionNode {
    ElementTest test;

    explicit ExistsElementNode(const ElementTest& test);
    bool eval(const Game& game) const override;
};

struct HasBeenPlayedNode : ConditionNode {
    MoveTest test;

    explicit HasBeenPlayedNode(const MoveTest& test);
    bool eval(const Game& game) const override;
};

struct IsLegalNode : ConditionNode {
    MoveTest test;

    explicit IsLegalNode(const MoveTest& test);
    bool eval(const Game& game) const override;
};

struct CompareIntNode : ConditionNode {
    enum Comparison { EQ, NEQ } op;

    IntExpr lhs;
    IntExpr rhs;

    CompareIntNode(const IntExpr& lhs, const IntExpr& rhs, Comparison op);
    bool eval(const Game& game) const override;
};

struct NotConditionNode : ConditionNode {
    Condition inner;

    explicit NotConditionNode(const Condition& inner);
    bool eval(const Game& game) const override;
};

struct AndConditionNode : ConditionNode {
    Condition a;
    Condition b;

    AndConditionNode(const Condition& a, const Condition& b);
    bool eval(const Game& game) const override;
};

struct OrConditionNode : ConditionNode {
    Condition a;
    Condition b;

    OrConditionNode(const Condition& a, const Condition& b);
    bool eval(const Game& game) const override;
};

struct CompareConditionsNode : ConditionNode {
    enum Comparison { EQ, NEQ } op;

    Condition lhs;
    Condition rhs;

    CompareConditionsNode(const Condition& lhs, const Condition& rhs, Comparison op);
    bool eval(const Game& game) const override;
};

struct LiteralIntNode : IntNode {
    int value;

    explicit LiteralIntNode(int value);
    int eval(const Game& game) const override;
};

struct CurrentMoveNode : IntNode {
    int eval(const Game& game) const override;
};

struct PreviousMoveNode : IntNode {
    int eval(const Game& game) const override;
};

struct CountElementsNode : IntNode {
    ElementTest test;

    explicit CountElementsNode(const ElementTest& test);
    int eval(const Game& game) const override;
};

struct MoveWhereNode : IntNode {
    MoveTest test;

    explicit MoveWhereNode(const MoveTest& test);
    int eval(const Game& game) const override;
};

struct AddIntNode : IntNode {
    IntExpr lhs;
    IntExpr rhs;

    AddIntNode(const IntExpr& lhs, const IntExpr& rhs);
    int eval(const Game& game) const override;
};

struct SubtractIntNode : IntNode {
    IntExpr lhs;
    IntExpr rhs;

    SubtractIntNode(const IntExpr& lhs, const IntExpr& rhs);
    int eval(const Game& game) const override;
};

struct Rule {
    Condition guard;
    MoveTest move;
    std::optional<std::string> name;
};

struct Strategy {
    std::vector<Rule> rules;
};

ElementTest picked_on_move(int move_number);
ElementTest picked_on_move(const IntExpr& move_number);
extern const ElementTest fail;
extern const ElementTest is_singleton;
extern const ElementTest are_singleton;
ElementTest operator~(const ElementTest& inner);
ElementTest operator&(const ElementTest& a, const ElementTest& b);
ElementTest operator|(const ElementTest& a, const ElementTest& b);

MoveTest all_elements(const ElementTest& test);
MoveTest all_that(const ElementTest& test);
MoveTest all_but(int n, const ElementTest& test);
MoveTest all_but(const IntExpr& n, const ElementTest& test);
MoveTest any_from(int n, const ElementTest& test);
MoveTest any_from(const IntExpr& n, const ElementTest& test);
MoveTest operator~(const MoveTest& inner);
MoveTest operator&(const MoveTest& a, const MoveTest& b);
MoveTest operator|(const MoveTest& a, const MoveTest& b);

extern const MoveTest anything;
extern const MoveTest nothing;
extern const MoveTest everything;

extern const Condition TRUE;
extern const Condition FALSE;
Condition true_condition();
Condition false_condition();
Condition there_is_an_element(const ElementTest& test);
Condition has_been_played(const MoveTest& test);
Condition is_legal(const MoveTest& test);
Condition operator!(const Condition& inner);
Condition operator&&(const Condition& a, const Condition& b);
Condition operator||(const Condition& a, const Condition& b);
Condition operator==(const IntExpr& lhs, const IntExpr& rhs);
Condition operator==(const IntExpr& lhs, int rhs);
Condition operator==(int lhs, const IntExpr& rhs);
Condition operator!=(const IntExpr& lhs, const IntExpr& rhs);
Condition operator!=(const IntExpr& lhs, int rhs);
Condition operator!=(int lhs, const IntExpr& rhs);
Condition operator==(const Condition& lhs, const Condition& rhs);
Condition operator!=(const Condition& lhs, const Condition& rhs);

IntExpr number_of_elements(const ElementTest& test);
IntExpr move_where(const MoveTest& test);
IntExpr operator+(const IntExpr& lhs, const IntExpr& rhs);
IntExpr operator+(const IntExpr& lhs, int rhs);
IntExpr operator+(int lhs, const IntExpr& rhs);
IntExpr operator-(const IntExpr& lhs, const IntExpr& rhs);
IntExpr operator-(const IntExpr& lhs, int rhs);
IntExpr operator-(int lhs, const IntExpr& rhs);
extern const IntExpr current_move;
extern const IntExpr previous_move;

struct ElementTestWhenBuilder {
    ElementTest a;
    Condition cond;

    ElementTest otherwise(const ElementTest& b) const;
};

struct MoveTestWhenBuilder {
    MoveTest a;
    Condition cond;

    MoveTest otherwise(const MoveTest& b) const;
};

struct IfFrame {
    int id;
    Condition parent;
    Condition cond;
    bool awaiting_else = false;
};

struct StrategyBuilder {
    Condition current = TRUE;
    std::vector<Rule> rules;
    std::vector<IfFrame> if_stack;
    int next_if_id = 0;

    void flush_pending_ifs();
    void flush_pending_ifs_above(int id);
    void pick(const MoveTest& m);
    void pick(const MoveTest& m, const std::string& name);
    Strategy finish() const;
};

struct IfScope {
    StrategyBuilder& builder;
    int id;

    IfScope(StrategyBuilder& builder, const Condition& cond);
    ~IfScope();

    explicit operator bool() const { return true; }
};

struct ElseScope {
    StrategyBuilder& builder;
    int id;

    explicit ElseScope(StrategyBuilder& builder);
    ~ElseScope();

    explicit operator bool() const { return true; }
};

#define IF(cond) if (IfScope _if_scope_{ builder, (cond) })
#define ELSE if (ElseScope _else_scope_{ builder })
#define PICK(...) builder.pick(__VA_ARGS__)

std::vector<Move> allowedMoves(const Strategy& strategy, const Game& position);
std::vector<Move> allowedLegalMoves(const Strategy& strategy, const Game& position);
std::vector<Move> allowedPrincipalLegalMoves(const Strategy& strategy, const Game& position);
std::optional<Move> firstAllowedLegalMove(const Strategy& strategy, const Game& position);
std::optional<std::string> ruleNameForMove(const Strategy& strategy, const Game& position, Move move);
void clearStrategyRuntimeError();
bool hasStrategyRuntimeError();
std::string strategyRuntimeErrorMessage();
void printAllowedMoves(const Strategy& strategy, const Game& position, std::ostream& out = std::cout);

struct StrategyVerificationResult {
    bool wins = false;
    std::vector<Move> line;
};

bool strategyWinsFrom(const Strategy& strategy, const Game& position, bool strategyPlayersTurn);
StrategyVerificationResult verifyStrategy(const Strategy& strategy, const Game& position, bool strategyPlayersTurn);
