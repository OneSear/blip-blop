#include <doctest.h>

#include <memory>
#include <string>
#include <vector>

#include "bb/game/screen.hpp"

namespace {

using bb::game::Frame;
using bb::game::Screen;
using bb::game::ScreenStack;

// A screen that follows a script of actions, one per update(), and logs
// what happens to it.
struct Scripted : Screen {
    enum class Action { Present, Wait, Finish, FinishAndPresent, PushChild, Stall };

    Scripted(std::string id, std::vector<Action> script, std::vector<std::string>& log)
        : id_(std::move(id)), script_(std::move(script)), log_(log) {}
    ~Scripted() override { log_.push_back(id_ + " destroyed"); }

    Frame update(ScreenStack& stack) override {
        const Action action = script_.at(next_++);
        log_.push_back(id_ + " update");
        switch (action) {
            case Action::Present:
                return Frame::Present;
            case Action::Wait:
                return Frame::Wait;
            case Action::Finish:
                stack.finish();
                // Still alive until update() returns.
                log_.push_back(id_ + " finishing");
                return Frame::Continue;
            case Action::FinishAndPresent:
                stack.finish();
                return Frame::Present;
            case Action::PushChild:
                stack.push(std::make_unique<Scripted>(
                    id_ + ".child", std::vector<Action>{Action::Present, Action::Finish}, log_));
                return Frame::Continue;
            case Action::Stall:
                return Frame::Continue;
        }
        return Frame::Wait;
    }
    const char* name() const override { return "scripted"; }

private:
    std::string id_;
    std::vector<Action> script_;
    std::vector<std::string>& log_;
    std::size_t next_ = 0;
};

using A = Scripted::Action;

}  // namespace

TEST_CASE("ScreenStack runs the top screen and resumes the one below when it finishes") {
    std::vector<std::string> log;
    ScreenStack stack;
    stack.push(std::make_unique<Scripted>("a", std::vector<A>{A::PushChild, A::Present, A::Finish}, log));

    CHECK(stack.step() == Frame::Continue);  // a pushes a.child
    CHECK(stack.size() == 2);
    CHECK(stack.step() == Frame::Present);   // a.child draws
    CHECK(stack.step() == Frame::Continue);  // a.child finishes
    CHECK(stack.size() == 1);
    CHECK(stack.step() == Frame::Present);   // a again
    CHECK(stack.step() == Frame::Continue);  // a finishes
    CHECK(stack.empty());
    CHECK(stack.stalledSteps() == 0);

    const std::vector<std::string> expected = {
        "a update", "a.child update", "a.child update", "a.child finishing", "a.child destroyed",
        "a update", "a update",       "a finishing",    "a destroyed",
    };
    CHECK(log == expected);
}

TEST_CASE("A screen can finish and present its last frame in the same step") {
    std::vector<std::string> log;
    ScreenStack stack;
    stack.push(std::make_unique<Scripted>("a", std::vector<A>{A::FinishAndPresent}, log));
    CHECK(stack.step() == Frame::Present);
    CHECK(stack.empty());
}

TEST_CASE("replace hands over to another screen; the one below resumes after it") {
    std::vector<std::string> log;
    ScreenStack stack;
    stack.push(std::make_unique<Scripted>("a", std::vector<A>{A::Present}, log));
    stack.push(std::make_unique<Scripted>("b", std::vector<A>{}, log));
    stack.replace(std::make_unique<Scripted>("c", std::vector<A>{A::Finish}, log));
    CHECK(stack.size() == 2);
    CHECK(stack.step() == Frame::Continue);  // c finishes; b was destroyed at the first step
    CHECK(stack.step() == Frame::Present);   // a
    const std::vector<std::string> expected = {"c update", "c finishing", "b destroyed", "c destroyed",
                                               "a update"};
    CHECK(log == expected);
}

TEST_CASE("Continue without a change of the stack is reported as a stall") {
    std::vector<std::string> log;
    ScreenStack stack;
    stack.push(std::make_unique<Scripted>("a", std::vector<A>{A::Stall, A::Wait}, log));
    CHECK(stack.step() == Frame::Wait);
    CHECK(stack.stalledSteps() == 1);
    CHECK(stack.step() == Frame::Wait);
    CHECK(stack.stalledSteps() == 1);
}

TEST_CASE("Destroying the stack destroys the screens from the top") {
    std::vector<std::string> log;
    {
        ScreenStack stack;
        stack.push(std::make_unique<Scripted>("a", std::vector<A>{}, log));
        stack.push(std::make_unique<Scripted>("b", std::vector<A>{}, log));
    }
    const std::vector<std::string> expected = {"b destroyed", "a destroyed"};
    CHECK(log == expected);
}
