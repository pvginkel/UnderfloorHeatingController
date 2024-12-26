#pragma once

template <typename Arg>
class Callback {
    static_assert((std::is_trivially_copyable<Arg>::value), "Arg must be trivially copyable");

    struct Node {
        function<void(Arg)> func;
        Node* next;

        Node(const function<void(Arg)>& func, Node* next) : func(func), next(next) {}
    };

    Node* _node;

public:
    Callback() : _node(nullptr) {}

    ~Callback() { clear(); }

    void add(const function<void(Arg)>& func) { _node = new Node(func, _node); }

    bool call(Arg arg) {
        auto node = _node;
        if (!node) {
            return false;
        }

        while (node) {
            node->func(arg);
            node = node->next;
        }

        return true;
    }

    void clear() {
        while (_node) {
            auto node = _node;
            _node = node->next;
            delete node;
        }
    }
};

template <>
class Callback<void> {
    struct Node {
        function<void()> func;
        Node* next;

        Node(const function<void()>& func, Node* next) : func(func), next(next) {}
    };

    Node* _node;

public:
    Callback() : _node(nullptr) {}

    ~Callback() { clear(); }

    void add(const function<void()>& func) { _node = new Node(func, _node); }

    bool call() {
        auto node = _node;
        if (!node) {
            return false;
        }

        while (node) {
            node->func();
            node = node->next;
        }

        return true;
    }

    void clear() {
        while (_node) {
            auto node = _node;
            _node = node->next;
            delete node;
        }
    }
};
