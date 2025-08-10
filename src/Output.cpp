#include "Output.h"

#include <string>

WiimoOscOutput::WiimoOscOutput() {
}

bool WiimoOscOutput::setup(const std::string & host, int port)
{
	if (mSender.getHost() == host && mSender.getPort() == port && mSender.isReady())
		return true;

	bool r = mSender.setup(host, port);
	if (r)
		ofLogNotice() << "OSC: Set up connection to " << host << ":" << port;
	else
		ofLogWarning() << "OSC: Failed to set up connection to " << host << ":" << port;
	return r;
}

bool WiimoOscOutput::processControllerEvents(const Wiimote::ControllerEvents & events)
{
	if (!mSender.isReady())
		return false;

	const auto makePrefix = [](int id) {
		using namespace std::string_literals;
		return "/wiimo/"s + std::to_string(id);
	};

	for (int i = 0; i < Wiimote::MoteButtonEnd; ++i) {
		if (auto t = events.moteButtonTransitions[i]; t != Wiimote::TransitionNone) {
			send(makePrefix(events.id) + "/mote/button/" + std::to_string(i), static_cast<bool>(t == Wiimote::TransitionPressed));
		}
	}

	if (events.moteOrientation) {
		auto & rpy = *events.moteOrientation;
		send(makePrefix(events.id) + "/mote/rpy", rpy.roll, rpy.pitch, rpy.yaw);
	}

	if (events.chuckJoystick) {
		auto & joy = *events.chuckJoystick;
		send(makePrefix(events.id) + "/chuck/joy", joy.angle, joy.magni, joy.x, joy.y);
	}

	if (events.balanceBoard) {
		auto & b = *events.balanceBoard;
		send(makePrefix(events.id) + "/board/xy", b.x, b.y);
		send(makePrefix(events.id) + "/board/raw", b.tr, b.tl, b.br, b.bl);
		send(makePrefix(events.id) + "/board/total", b.total);
	}

	return true;
}


