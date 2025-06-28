#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <list>

namespace state_machine {

// Context handles resumption of current coroutine stack
class Context {
public:
    explicit Context(bool should_initialy_suspend): top_(), should_initialy_suspend_(should_initialy_suspend) {}

    // We pass Context by reference so it cannot be moved
    Context(Context&&) = delete;

    // Save handle as top of the stack for resumption
    void push(std::coroutine_handle<> h) {
        // Only first one in stack should initialy suspend
        this->should_initialy_suspend_ = false;
        // Top of the stack will be pushed first, so save only if empty
        if (this->empty()) this->top_ = h;
    }

    std::coroutine_handle<> top() const {
        if (!this->top_) return std::noop_coroutine();
        return this->top_;
    }

    void resume() {
        if (this->top_) {
            auto tmp = this->top_;
            this->top_ = {};
            tmp.resume();
        }
    }

    bool empty() const { return !this->top_; }

    // True is coroutine is not first in queue, so it is not allowed to start yet
    bool should_initialy_suspend() const { return this->should_initialy_suspend_; }
private:
    std::coroutine_handle<> top_;
    bool should_initialy_suspend_;
};

template<class T>
class Promise;

template<class T>
class Coroutine : public std::coroutine_handle<Promise<T>> {
public:
    using promise_type = Promise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Coroutine(): owning_(false) {}
    Coroutine(handle_type h): handle_type(h), owning_(true) {}
    ~Coroutine() {
        // Free coroutine state
        if (this->owning_) this->destroy();
    }

    Coroutine(Coroutine&& other): handle_type(std::move(other)), owning_(other.owning_) { other.owning_ = false; }
    Coroutine& operator=(Coroutine&& other) {
        if (this->owning_) this->destroy();

        this->handle_type::operator=(std::move(other));

        this->owning_ = other.owning_;
        other.owning_ = false;
        return *this;
    }

private:
    bool owning_;
};

template<class T>
class Value {
public:
    void return_value(const T& val) { this->val_ = val; }
    void return_value(T&& val) { this->val_ = std::move(val); }
    T get_value() { return std::move(this->val_); }

private:
    T val_;
};

template<>
class Value<void> {
public:
    void return_void() {}
    void get_value() {}
};

class InitialAwaiter {
public:
    InitialAwaiter(Context& ctx): ctx_(ctx) {}
    bool await_ready() const noexcept { return !this->ctx_.should_initialy_suspend(); }
    void await_suspend(std::coroutine_handle<> h) noexcept { this->ctx_.push(h); }
    void await_resume() noexcept {}
private:
    Context& ctx_;
};

template<class T>
class CoroutineAwaiter {
public:
    CoroutineAwaiter(const Coroutine<T>& h): h_(h) {}

    bool await_ready() const noexcept { return this->h_.done(); }

    void await_suspend(std::coroutine_handle<> h) noexcept {
        auto&& p = this->h_.promise();
        // Save current coroutine as parent for awaited coroutine for returning to
        p.parent_ = h;
        // No need to put current coroutine in stack as it is not the top,
        // as awaited coroutine is suspended and is higher in stack
    }

    T await_resume() noexcept {
        return this->h_.promise().get_value();
    }

private:
    Coroutine<T>::handle_type h_;
};

class CoroutineFinalAwaiter {
public:
    CoroutineFinalAwaiter(std::coroutine_handle<> h): h_(h) {}

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept { return this->h_; }
    void await_resume() const noexcept {}

private:
    std::coroutine_handle<> h_;
};

template<class T>
class Promise : public Value<T> {
public:
    template<class ...Args>
    Promise(Context& ctx, Args&&...): ctx_(ctx) {}

    template<class This, class ...Args>
    Promise(This&&, Context& ctx, Args&&...): ctx_(ctx) {}

    Coroutine<T> get_return_object() {
        return Coroutine(std::coroutine_handle<Promise<T>>::from_promise(*this));
    }
    InitialAwaiter initial_suspend() noexcept { return InitialAwaiter(this->ctx_); }

    CoroutineFinalAwaiter final_suspend() noexcept { return CoroutineFinalAwaiter(this->parent_); }

    void unhandled_exception() {
        // TODO report exception
        std::terminate();
    }

    // coroutines can be awaited, this forms a stack as expected
    template<class Y>
    auto await_transform(const Coroutine<Y>& coro) {
        return CoroutineAwaiter(coro);
    }

    // std::suspend_always can be awaited, result is unconditional suspend
    std::suspend_always await_transform(std::suspend_always) {
        // Put current coroutine to stack and suspend
        ctx_.push(std::coroutine_handle<Promise<T>>::from_promise(*this));
        return {};
    }

private:
    Context& ctx_;
    // Parent coroutine handle to return to
    std::coroutine_handle<> parent_ = std::noop_coroutine();

    template<class Y>
    friend class CoroutineAwaiter;
};

class Queue {
public:
    using func = std::function<Coroutine<void>(Context&)>;
private:
    struct Task {
        // Save functional object to extend it's lifetime
        func f_;
        Context ctx_;
        // Hold the bottommost handle of the stack, the rest are held at the respective call sites
        Coroutine<void> h_;

        Task(bool should_initialy_suspend, func&& f)
            : f_(std::move(f))
            , ctx_(should_initialy_suspend)
        {}

        void start() {
            this->h_ = this->f_(this->ctx_);
        }
    };

public:
    Queue() = default;
    //Queue(size_t size_limit): size_limit_(size_limit) {}

    // Enqueue coroutine, start it inside this call if it is the first in the queue
    void enqueue(func f) {
        //TODO check the limit

        // Create a coroutine,
        // allow it to start only if it is the first in the queue
        auto allow_eager_start = this->empty();
        auto&& task = this->task_queue_.emplace_back(!allow_eager_start, std::move(f));

        // Start the coroutine
        task.start();

        // If the current coroutine was eagerly started and is done try to resume coroutines enqueued by it,
        // this also cleans it up
        if (allow_eager_start && task.h_.done()) {
            poll();
        }
    }

    // Resume waiting coroutines until one suspends
    void poll() {
        while (!this->empty()) {
            auto& task = this->task_queue_.front();
            task.ctx_.resume();

            // We are done for now if current coroutine is suspended,
            // otherwise remove it and continue to the next
            if (!task.ctx_.empty()) break;
            this->task_queue_.pop_front();
        }
    }

    bool empty() const {
        return this->task_queue_.empty();
    }

private:
    std::list<Task> task_queue_;
    //size_t size_limit_ = 16;
};

}// namespace state_machine
