#pragma once

#include "takeaway.h" // Include the game logic and solver core for the takeaway game
#include <memory>     // Shared pointers for AST nodes

// We want to formalize what a game strategy actually means
// This is where things get complicated
// It will be represented as an AST (Abstract Syntax Tree)
// This AST will represent the rules to follow in order to pick a move from a given game position

// Create a shorthand for a shared pointer
// It allows the same node to be referenced from multiple locations
template<typename Node>
using Ptr = std::shared_ptr<Node>;

// Define the main node types
struct ElementTestNode;
struct MoveTestNode;
struct ConditionNode;
struct IntNode;

// Define the main symbolic wrapper types
struct ElementTest;
struct MoveTest;
struct Condition;
struct IntExpr;

// Define the conditional helper builder types
struct ElementTestWhenBuilder;
struct MoveTestWhenBuilder;

// These are the main symbolic wrapper types
// They simply point to their corresponding AST node
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

// Evaluators
bool eval(const ElementTest&, const Game&, Element); // Element test evaluator
bool eval(const MoveTest&, const Game&, Move);       // Move test evaluator
bool eval(const Condition&, const Game&);            // Condition evaluator
int  eval(const IntExpr&, const Game&);              // Integer evaluator

// We will need to populate this tree with many different types of nodes
// The first type of syntax node we need is tests

// We will make tests for elements and moves
// A given element or move either fails or passes the test
struct ElementTestNode {
    virtual ~ElementTestNode() = default;
    virtual bool eval(const Game& game, Element element) const = 0;
};

struct MoveTestNode {
    virtual ~MoveTestNode() = default;
    virtual bool eval(const Game& game, Move move) const = 0;
};

// Conditions are boolean expressions that can depend on the current game position
// A condition is either met (true) or not met (false)
struct ConditionNode {
    virtual ~ConditionNode() = default;
    virtual bool eval(const Game& game) const = 0;
};

// Integer expressions are values that can depend on the current game position
struct IntNode {
    virtual ~IntNode() = default;
    virtual int eval(const Game& game) const = 0;
};



// Tests for elements

// A test that passes if the element was picked on a specific move
struct PickedOnMoveNode : ElementTestNode {
    int move_number;

    PickedOnMoveNode(int move_number) : move_number(move_number) {}

    bool eval(const Game& game, Element element) const override {
        if (move_number < 1 || move_number > game.size()) {
            return false;
        }
        return ManipulateMove::hasElement(game[move_number - 1], element);
    }
};



// Tests for moves

// Constant tests

// A test that passes if the move is legal
struct AnythingNode : MoveTestNode {
    bool eval(const Game& game, Move move) const override {
        return game.isMoveLegal(move);
    }
};

// A test that always fails
struct NothingNode : MoveTestNode {
    bool eval(const Game&, Move) const override {
        return false;
    }
};

// A test that only passes for the move that picks every element in E
struct EverythingNode : MoveTestNode {
    bool eval(const Game& game, Move move) const override {
        return move == game.E.bitmask;
    }
};


// Element based tests

// A test that passes if the move is exactly the set of all elements that pass the given element test
struct AllElementsNode : MoveTestNode {
    ElementTest test;

    AllElementsNode(const ElementTest& test) : test(test) {}

    bool eval(const Game& game, Move move) const override {
        for (Element element = 0; element < game.E.size; element++) {
            bool picked = ManipulateMove::hasElement(move, element);
            bool passes = ::eval(test, game, element);
            if (picked != passes) {
                return false;
            }
        }
        return true;
    }
};

// A test that passes if exactly n picked elements pass the given element test
struct AnyFromNode : MoveTestNode {
    int n;
    ElementTest test;

    AnyFromNode(int n, const ElementTest& test) : n(n), test(test) {}

    bool eval(const Game& game, Move move) const override {
        int count = 0;
        for (Element element = 0; element < game.E.size; element++) {
            if (ManipulateMove::hasElement(move, element) && ::eval(test, game, element)) {
                count++;
            }
        }
        return count == n;
    }
};



// Test logic operators

// Tests that pass if the inner test fails and fail if the inner test passes
struct NotElementTestNode : ElementTestNode {
    ElementTest inner;

    NotElementTestNode(const ElementTest& inner) : inner(inner) {}

    bool eval(const Game& game, Element element) const override {
        return !::eval(inner, game, element);
    }
};

struct NotMoveTestNode : MoveTestNode {
    MoveTest inner;

    NotMoveTestNode(const MoveTest& inner) : inner(inner) {}

    bool eval(const Game& game, Move move) const override {
        return !::eval(inner, game, move);
    }
};

// Tests that only pass if both inner tests pass
struct AndElementTestNode : ElementTestNode {
    ElementTest a, b;

    AndElementTestNode(const ElementTest& a, const ElementTest& b) : a(a), b(b) {}

    bool eval(const Game& game, Element element) const override {
        return ::eval(a, game, element) && ::eval(b, game, element);
    }
};

struct AndMoveTestNode : MoveTestNode {
    MoveTest a, b;

    AndMoveTestNode(const MoveTest& a, const MoveTest& b) : a(a), b(b) {}

    bool eval(const Game& game, Move move) const override {
        return ::eval(a, game, move) && ::eval(b, game, move);
    }
};

// Tests that pass if any inner test passes
struct OrElementTestNode : ElementTestNode {
    ElementTest a, b;

    OrElementTestNode(const ElementTest& a, const ElementTest& b) : a(a), b(b) {}

    bool eval(const Game& game, Element element) const override {
        return ::eval(a, game, element) || ::eval(b, game, element);
    }
};

struct OrMoveTestNode : MoveTestNode {
    MoveTest a, b;

    OrMoveTestNode(const MoveTest& a, const MoveTest& b) : a(a), b(b) {}

    bool eval(const Game& game, Move move) const override {
        return ::eval(a, game, move) || ::eval(b, game, move);
    }
};

// Conditional test nodes
// Acts like the first test when the condition is met and the second when it is not
struct SelectElementTestNode : ElementTestNode {
    Condition cond;
    ElementTest a, b;

    SelectElementTestNode(const Condition& cond, const ElementTest& a, const ElementTest& b)
        : cond(cond), a(a), b(b) {
    }

    bool eval(const Game& game, Element element) const override {
        return ::eval(cond, game)
            ? ::eval(a, game, element)
            : ::eval(b, game, element);
    }
};

struct SelectMoveTestNode : MoveTestNode {
    Condition cond;
    MoveTest a, b;

    SelectMoveTestNode(const Condition& cond, const MoveTest& a, const MoveTest& b)
        : cond(cond), a(a), b(b) {
    }

    bool eval(const Game& game, Move move) const override {
        return ::eval(cond, game)
            ? ::eval(a, game, move)
            : ::eval(b, game, move);
    }
};



// Constant conditions

// A condition that is always met
struct TrueNode : ConditionNode {
    bool eval(const Game&) const override {
        return true;
    }
};

// A condition that is never met
struct FalseNode : ConditionNode {
    bool eval(const Game&) const override {
        return false;
    }
};

// Game position conditions

// The condition that there is at least one element that passes a given element test
struct ExistsElementNode : ConditionNode {
    ElementTest test;

    ExistsElementNode(const ElementTest& test) : test(test) {}

    bool eval(const Game& game) const override {
        for (Element element = 0; element < game.E.size; element++) {
            if (::eval(test, game, element)) {
                return true;
            }
        }
        return false;
    }
};

// The condition that two integer expressions are equal or not equal
struct CompareIntNode : ConditionNode {
    IntExpr lhs;
    IntExpr rhs;
    enum { EQ, NEQ } op;

    CompareIntNode(const IntExpr& lhs, const IntExpr& rhs, int op)
        : lhs(lhs), rhs(rhs), op(static_cast<decltype(this->op)>(op)) {
    }

    bool eval(const Game& game) const override {
        int left = ::eval(lhs, game);
        int right = ::eval(rhs, game);
        return op == EQ ? (left == right) : (left != right);
    }
};

// Condition logic operators

// Returns the opposite condition
struct NotConditionNode : ConditionNode {
    Condition inner;

    NotConditionNode(const Condition& inner) : inner(inner) {}

    bool eval(const Game& game) const override {
        return !::eval(inner, game);
    }
};

// A condition that is only met if both inner conditions are met
struct AndConditionNode : ConditionNode {
    Condition a, b;

    AndConditionNode(const Condition& a, const Condition& b) : a(a), b(b) {}

    bool eval(const Game& game) const override {
        return ::eval(a, game) && ::eval(b, game);
    }
};

// A condition that is met if any inner condition is met
struct OrConditionNode : ConditionNode {
    Condition a, b;

    OrConditionNode(const Condition& a, const Condition& b) : a(a), b(b) {}

    bool eval(const Game& game) const override {
        return ::eval(a, game) || ::eval(b, game);
    }
};

// A condition that checks whether two conditions evaluate the same or differently
struct CompareConditionsNode : ConditionNode {
    Condition lhs;
    Condition rhs;
    enum { EQ, NEQ } op;

    CompareConditionsNode(const Condition& lhs, const Condition& rhs, int op)
        : lhs(lhs), rhs(rhs), op(static_cast<decltype(this->op)>(op)) {
    }

    bool eval(const Game& game) const override {
        bool left = ::eval(lhs, game);
        bool right = ::eval(rhs, game);
        return op == EQ ? (left == right) : (left != right);
    }
};



// Constant integer
struct LiteralIntNode : IntNode {
    int value;

    LiteralIntNode(int value) : value(value) {}

    int eval(const Game&) const override {
        return value;
    }
};

// An integer representing how many elements pass an element test
struct CountElementsNode : IntNode {
    ElementTest test;

    CountElementsNode(const ElementTest& test) : test(test) {}

    int eval(const Game& game) const override {
        int count = 0;
        for (Element element = 0; element < game.E.size; element++) {
            if (::eval(test, game, element)) {
                count++;
            }
        }
        return count;
    }
};



// Strategy definitions

// A rule is a move test with a guard condition that must be met to activate it
struct Rule {
    Condition guard;
    MoveTest move;
};

// A strategy is a list of rules, a move is allowed if it satisfies at least one active rule
struct Strategy {
    std::vector<Rule> rules;
};



// Element tests
inline ElementTest picked_on_move(int move_number) {
    return ElementTest{ std::make_shared<PickedOnMoveNode>(move_number) };
}

// Element test operations
inline ElementTest operator~(const ElementTest& inner) {
    return ElementTest{ std::make_shared<NotElementTestNode>(inner) };
}

inline ElementTest operator&(const ElementTest& a, const ElementTest& b) {
    return ElementTest{ std::make_shared<AndElementTestNode>(a, b) };
}

inline ElementTest operator|(const ElementTest& a, const ElementTest& b) {
    return ElementTest{ std::make_shared<OrElementTestNode>(a, b) };
}



// Move tests
inline MoveTest all_elements(const ElementTest& test) {
    return MoveTest{ std::make_shared<AllElementsNode>(test) };
}

inline MoveTest any_from(int n, const ElementTest& test) {
    return MoveTest{ std::make_shared<AnyFromNode>(n, test) };
}

// Move test operations
inline MoveTest operator~(const MoveTest& inner) {
    return MoveTest{ std::make_shared<NotMoveTestNode>(inner) };
}

inline MoveTest operator&(const MoveTest& a, const MoveTest& b) {
    return MoveTest{ std::make_shared<AndMoveTestNode>(a, b) };
}

inline MoveTest operator|(const MoveTest& a, const MoveTest& b) {
    return MoveTest{ std::make_shared<OrMoveTestNode>(a, b) };
}

// Constant move tests
inline const MoveTest anything = MoveTest{ std::make_shared<AnythingNode>() };
inline const MoveTest nothing = MoveTest{ std::make_shared<NothingNode>() };
inline const MoveTest everything = MoveTest{ std::make_shared<EverythingNode>() };



// Conditions
inline Condition true_condition() {
    return Condition{ std::make_shared<TrueNode>() };
}

inline Condition false_condition() {
    return Condition{ std::make_shared<FalseNode>() };
}

inline Condition there_is_an_element(const ElementTest& test) {
    return Condition{ std::make_shared<ExistsElementNode>(test) };
}

// Condition operations
inline Condition operator!(const Condition& inner) {
    return Condition{ std::make_shared<NotConditionNode>(inner) };
}

inline Condition operator&&(const Condition& a, const Condition& b) {
    return Condition{ std::make_shared<AndConditionNode>(a, b) };
}

inline Condition operator||(const Condition& a, const Condition& b) {
    return Condition{ std::make_shared<OrConditionNode>(a, b) };
}

inline Condition operator==(const IntExpr& lhs, int rhs) {
    return Condition{
        std::make_shared<CompareIntNode>(
            lhs,
            IntExpr{ std::make_shared<LiteralIntNode>(rhs) },
            CompareIntNode::EQ
        )
    };
}

inline Condition operator!=(const IntExpr& lhs, int rhs) {
    return Condition{
        std::make_shared<CompareIntNode>(
            lhs,
            IntExpr{ std::make_shared<LiteralIntNode>(rhs) },
            CompareIntNode::NEQ
        )
    };
}

inline Condition operator==(const Condition& lhs, const Condition& rhs) {
    return Condition{ std::make_shared<CompareConditionsNode>(lhs, rhs, CompareConditionsNode::EQ) };
}

inline Condition operator!=(const Condition& lhs, const Condition& rhs) {
    return Condition{ std::make_shared<CompareConditionsNode>(lhs, rhs, CompareConditionsNode::NEQ) };
}



// Integer expressions
inline IntExpr number_of_elements(const ElementTest& test) {
    return IntExpr{ std::make_shared<CountElementsNode>(test) };
}



// Conditional helpers

struct ElementTestWhenBuilder {
    ElementTest a;
    Condition cond;

    ElementTest otherwise(const ElementTest& b) const {
        return ElementTest{ std::make_shared<SelectElementTestNode>(cond, a, b) };
    }
};

struct MoveTestWhenBuilder {
    MoveTest a;
    Condition cond;

    MoveTest otherwise(const MoveTest& b) const {
        return MoveTest{ std::make_shared<SelectMoveTestNode>(cond, a, b) };
    }
};

inline ElementTestWhenBuilder ElementTest::when(const Condition& cond) const {
    return ElementTestWhenBuilder{ *this, cond };
}

inline MoveTestWhenBuilder MoveTest::when(const Condition& cond) const {
    return MoveTestWhenBuilder{ *this, cond };
}



// Stores state to help build the strategy AST

struct IfFrame {
    int id;
    Condition parent;
    Condition cond;
};

struct StrategyBuilder {
    Condition current = true_condition();
    std::vector<Rule> rules;
    std::vector<IfFrame> if_stack;
    int next_if_id = 0;

    void pick(const MoveTest& m) {
        rules.push_back({ current, m });
    }

    Strategy finish() const {
        return Strategy{ rules };
    }
};



// Control structure scopes

struct IfScope {
    StrategyBuilder& builder;
    int id;

    IfScope(StrategyBuilder& builder, const Condition& cond)
        : builder(builder), id(builder.next_if_id++) {
        builder.if_stack.push_back({ id, builder.current, cond });
        builder.current = builder.current && cond;
    }

    ~IfScope() {
        if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
            builder.current = builder.if_stack.back().parent;
            builder.if_stack.pop_back();
        }
    }

    explicit operator bool() const { return true; }
};

struct ElseScope {
    StrategyBuilder& builder;
    int id;

    ElseScope(StrategyBuilder& builder)
        : builder(builder), id(builder.if_stack.back().id) {
        const IfFrame& frame = builder.if_stack.back();
        builder.current = frame.parent && !frame.cond;
    }

    ~ElseScope() {
        if (!builder.if_stack.empty() && builder.if_stack.back().id == id) {
            builder.current = builder.if_stack.back().parent;
            builder.if_stack.pop_back();
        }
    }

    explicit operator bool() const { return true; }
};

struct WhileLegalScope {
    StrategyBuilder& builder;

    WhileLegalScope(StrategyBuilder& builder) : builder(builder) {}

    explicit operator bool() const { return true; }
};



// Control structure macros
#define IF(cond) if (IfScope _if_scope_(builder, (cond)))
#define ELSE     else if (ElseScope _else_scope_(builder))
#define PICK(x)  builder.pick(x)
#define WHILE_LEGAL if (WhileLegalScope _while_legal_scope_(builder))



// Evaluators

inline bool eval(const ElementTest& test, const Game& game, Element element) {
    return test.ptr->eval(game, element);
}

inline bool eval(const MoveTest& test, const Game& game, Move move) {
    return test.ptr->eval(game, move);
}

inline bool eval(const Condition& condition, const Game& game) {
    return condition.ptr->eval(game);
}

inline int eval(const IntExpr& expr, const Game& game) {
    return expr.ptr->eval(game);
}