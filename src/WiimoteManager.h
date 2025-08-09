#pragma once

#include <array>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <optional>
#include <functional>

#define MAX_WIIMOTES 4

namespace Wiimote
{

class Worker;

struct Orientation
{
    float pitch = 0.0;
    float roll = 0.0;
    float yaw = 0.0;
};

struct Joystick
{
    float angle = 0.0;
    float magni = 0.0;
    float x = 0.0;
    float y = 0.0;
};

struct BalanceBoard
{
	float x = 0.0;
	float y = 0.0;
	float total = 0.0;
	float tr = 0.0;
	float tl = 0.0;
	float br = 0.0;
	float bl = 0.0;
};

enum MoteButton
{
    MoteButtonBegin = 0,

	MoteButton_One = MoteButtonBegin,
	MoteButton_Two,
	MoteButton_B,
	MoteButton_A,
    MoteButton_Up,
    MoteButton_Down,
    MoteButton_Left,
    MoteButton_Right,
    MoteButton_Minus,
    MoteButton_Plus,
    MoteButton_Home,

    MoteButtonEnd
};

enum ChuckButton
{
	ChuckButtonBegin = 0,

	ChuchButton_C,
	ChuchButton_Z,

	ChuckButtonEnd
};

enum Transition
{
    TransitionNone,
    TransitionPressed,
    TransitionReleased,
};

struct ControllerEvents
{
    int id = 0;

    std::optional<Orientation> moteOrientation; 
    std::optional<Orientation> chuckOrientation;
	std::optional<Joystick> chuckJoystick;

    std::array<Transition, static_cast<size_t>(MoteButton::MoteButtonEnd)> moteButtonTransitions;

    std::optional<BalanceBoard> balanceBoard;
};

class Manager {
public:
    Manager();
    ~Manager();

	static std::optional<int> buttonToWiimoteCode(MoteButton button);

    void init();
    void update();

    void onControllerEvents(std::function<void(const ControllerEvents&)> callback) {
        mCallback = callback;
    }

private:
    std::unique_ptr<Worker> mWorker;
    std::optional<std::thread> mWorkerThread;

    std::mutex mEventMutex;
    std::array<std::optional<ControllerEvents>, MAX_WIIMOTES> mEvents;

    std::function<void(const ControllerEvents&)> mCallback = {};

    friend class Worker;
};

} // namespace Wiimote
