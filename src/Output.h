#include <ofxOsc.h>
#include <ofxMidi.h>

#include <type_traits>

#include "WiimoteManager.h"

class WiimoOscOutput
{
	ofxOscSender mSender;

	template <typename Arg>
	void makeMessage(ofxOscMessage& msg, Arg&& arg)
	{
		using T = std::decay_t<Arg>;

		if constexpr (std::is_same<T, bool>::value) {
			msg.addBoolArg(std::forward<Arg>(arg));
		}
		else if constexpr (std::is_integral<T>::value) {
			msg.addInt32Arg(std::forward<Arg>(arg));
		}
		else if constexpr (std::is_floating_point<T>::value) {
			msg.addFloatArg(std::forward<Arg>(arg));
		}
		else {
			static_assert(std::is_same_v<int, bool>, "WiimoOscOutput::send: Unhandled argument type.");
		}
	}

	template <typename Arg, typename... Args>
	void makeMessage(ofxOscMessage & msg, Arg&& arg, Args &&... args)
	{
		makeMessage(msg, std::forward<Arg>(arg));
		makeMessage(msg, std::forward<Args>(args)...);
	}

	template <typename... Args>
	bool send(const std::string& addr, Args &&... args)
	{
		ofxOscMessage msg;
		msg.setAddress(addr);
		makeMessage(msg, std::forward<Args>(args)...);
		return mSender.sendMessage(msg, false);
	}

public:
	WiimoOscOutput();

	bool setup(const std::string & host, int port);
	bool processControllerEvents(const Wiimote::ControllerEvents & events);
};
