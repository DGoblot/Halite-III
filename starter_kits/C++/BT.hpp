#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "Context.hpp"

//general behaviour tree class, credits to Axel Buendia for the code

class BT_NODE {
public:
    enum class State {
        RUNNING,
        SUCCESS,
        FAILURE
    };

    virtual ~BT_NODE() = default;

    virtual State evaluate(Context& ctx) = 0;
};

using NodePtr = std::unique_ptr<BT_NODE>;

class BT_SEQUENCER : public BT_NODE {
private:
    std::vector<NodePtr> children;

public:
    BT_SEQUENCER() = default;

    void addChild(NodePtr child) {
        children.push_back(std::move(child));
    }

    State evaluate(Context& ctx) override {
        for (auto& child : children) {
            State state = child->evaluate(ctx);
            if (state == State::FAILURE)
                return State::FAILURE;
            if (state == State::RUNNING)
                return State::RUNNING;
        }
        return State::SUCCESS;
    }
};

class BT_SELECTOR : public BT_NODE {
private:
    std::vector<NodePtr> children;

public:
    BT_SELECTOR() = default;

    void addChild(NodePtr child) {
        children.push_back(std::move(child));
    }

    State evaluate(Context& ctx) override {
        for (auto& child : children) {
            State state = child->evaluate(ctx);
            if (state == State::SUCCESS)
                return State::SUCCESS;
            if (state == State::RUNNING)
                return State::RUNNING;
        }
        return State::FAILURE;
    }
};

class BT_LEAF : public BT_NODE {
private:
    std::function<State(Context&)> fn;

public:
    explicit BT_LEAF(std::function<State(Context&)> f)
        : fn(std::move(f)) {
    }

    State evaluate(Context& ctx) override {
        return fn(ctx);
    }
};

inline NodePtr leaf(std::function<BT_NODE::State(Context&)> fn) {
    return std::make_unique<BT_LEAF>(std::move(fn));
}

inline NodePtr selector(std::initializer_list<NodePtr> children) {
    auto node = std::make_unique<BT_SELECTOR>();

    for (auto& child : children) {
        node->addChild(std::move(const_cast<NodePtr&>(child)));
    }

    return node;
}

inline NodePtr sequencer(std::initializer_list<NodePtr> children) {
    auto node = std::make_unique<BT_SEQUENCER>();

    for (auto& child : children) {
        node->addChild(std::move(const_cast<NodePtr&>(child)));
    }

    return node;
}