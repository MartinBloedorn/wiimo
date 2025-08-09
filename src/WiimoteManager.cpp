#include "WiimoteManager.h"

#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1

#include "wiiuse.h"

#include "ofLog.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <map>

//==============================================================================
//
// Worker
//
//==============================================================================

namespace Wiimote
{

template <typename... Args>
void logVerbose(const char* fmt, Args &&... args)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
	ofLogVerbose() << buffer;
}

class Worker 
{
public:
	Worker(Manager& manager)
		: mManager(manager)
	{
	}

	~Worker()
	{
		if (mWiimotes) {
			//("Cleaning up...");
			wiiuse_cleanup(mWiimotes, MAX_WIIMOTES);
		}
	}

	/**
	 *	@brief Callback that handles a read event.
	 *
	 *	@param wm		Pointer to a wiimote_t structure.
	 *	@param data		Pointer to the filled data block.
	 *	@param len		Length in bytes of the data block.
	 *
	 *	This function is called automatically by the wiiuse library when
	 *	the wiimote has returned the full data requested by a previous
	 *	call to wiiuse_read_data().
	 *
	 *	You can read data on the wiimote, such as Mii data, if
	 *	you know the offset address and the length.
	 *
	 *	The \a data pointer was specified on the call to wiiuse_read_data().
	 *	At the time of this function being called, it is not safe to deallocate
	 *	this buffer.
	 */
	void handle_read(struct wiimote_t* wm, byte* data, unsigned short len) {
		int i = 0;

		logVerbose("\n\n--- DATA READ [wiimote id %i] ---", wm->unid);
		logVerbose("finished read of size %i", len);
		for (; i < len; ++i) {
			if (!(i % 16)) {
				logVerbose("");
			}
			logVerbose("%x ", data[i]);
		}
		logVerbose("\n");
	}

	/**
	 *	@brief Callback that handles a controller status event.
	 *
	 *	@param wm				Pointer to a wiimote_t structure.
	 *	@param attachment		Is there an attachment? (1 for yes, 0 for no)
	 *	@param speaker			Is the speaker enabled? (1 for yes, 0 for no)
	 *	@param ir				Is the IR support enabled? (1 for yes, 0 for no)
	 *	@param led				What LEDs are lit.
	 *	@param battery_level	Battery level, between 0.0 (0%) and 1.0 (100%).
	 *
	 *	This occurs when either the controller status changed
	 *	or the controller status was requested explicitly by
	 *	wiiuse_status().
	 *
	 *	One reason the status can change is if the nunchuk was
	 *	inserted or removed from the expansion port.
	 */
	void handle_ctrl_status(struct wiimote_t* wm) {
		logVerbose("\n\n--- CONTROLLER STATUS [wiimote id %i] ---", wm->unid);

		logVerbose("attachment:      %i", wm->exp.type);
		logVerbose("speaker:         %i", WIIUSE_USING_SPEAKER(wm));
		logVerbose("ir:              %i", WIIUSE_USING_IR(wm));
		logVerbose("leds:            %i %i %i %i", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2), WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
		logVerbose("battery:         %f %%", wm->battery_level);
	}


	/**
	 *	@brief Callback that handles a disconnection event.
	 *
	 *	@param wm				Pointer to a wiimote_t structure.
	 *
	 *	This can happen if the POWER button is pressed, or
	 *	if the connection is interrupted.
	 */
	void handle_disconnect(wiimote* wm) {
		logVerbose("\n\n--- DISCONNECTED [wiimote id %i] ---", wm->unid);
	}

	/**
	 *	@brief Callback that handles an event.
	 *
	 *	@param wm		Pointer to a wiimote_t structure.
	 *
	 *	This function is called automatically by the wiiuse library when an
	 *	event occurs on the specified wiimote.
	 */
	void handle_event(struct wiimote_t* wm) {
		logVerbose("\n\n--- EVENT [id %i] ---", wm->unid);

        ControllerEvents events;
        events.id = wm->unid;

        for (int b = MoteButtonBegin; b < MoteButtonEnd; ++b)
        {
			auto code = Manager::buttonToWiimoteCode(MoteButton(b));

			if (!code.has_value())
				continue;

            if (IS_JUST_PRESSED(wm, *code)) {
				events.moteButtonTransitions[b] = TransitionPressed;
                ofLogVerbose() << "Button " << b << " pressed.";
            }
            else if (IS_RELEASED(wm, *code)) {
				events.moteButtonTransitions[b] = TransitionReleased;
                ofLogVerbose() << "Button " << b << " released.";
            }
            else {
				events.moteButtonTransitions[b] = TransitionNone;
            }
        }

		/* if a button is pressed, report it */
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_A)) {
			logVerbose("A pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_B)) {
			logVerbose("B pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_UP)) {
			logVerbose("UP pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN)) {
			logVerbose("DOWN pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT)) {
			logVerbose("LEFT pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT)) {
			logVerbose("RIGHT pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS)) {
			logVerbose("MINUS pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS)) {
			logVerbose("PLUS pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
			logVerbose("ONE pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
			logVerbose("TWO pressed");
		}
		if (IS_PRESSED(wm, WIIMOTE_BUTTON_HOME)) {
			logVerbose("HOME pressed");
		}

		/*
		 *	Pressing minus will tell the wiimote we are no longer interested in movement.
		 *	This is useful because it saves battery power.
		 */
		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS)) {
			wiiuse_motion_sensing(wm, 0);
		}

		/*
		 *	Pressing plus will tell the wiimote we are interested in movement.
		 */
		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS)) {
			wiiuse_motion_sensing(wm, 1);
		}

		/*
		 *	Pressing B will toggle the rumble
		 *
		 *	if B is pressed but is not held, toggle the rumble
		 */
		//if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_B)) {
		//	wiiuse_toggle_rumble(wm);
		//}

		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_UP)) {
			wiiuse_set_ir(wm, 1);
		}
		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_DOWN)) {
			wiiuse_set_ir(wm, 0);
		}

		/*
		 * Motion+ support
		 */
		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
			if (WIIUSE_USING_EXP(wm)) {
				wiiuse_set_motion_plus(wm, 2);    // nunchuck pass-through
			}
			else {
				wiiuse_set_motion_plus(wm, 1);    // standalone
			}
		}

		if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
			wiiuse_set_motion_plus(wm, 0); // off
		}

		/* if the accelerometer is turned on then print angles */
		if (WIIUSE_USING_ACC(wm)) {
			logVerbose("wiimote roll  = %f [%f]", wm->orient.roll, wm->orient.a_roll);
			logVerbose("wiimote pitch = %f [%f]", wm->orient.pitch, wm->orient.a_pitch);
			logVerbose("wiimote yaw   = %f", wm->orient.yaw);

			Orientation rpy;
			rpy.roll  = wm->orient.roll;
			rpy.pitch = wm->orient.pitch;
			rpy.yaw   = wm->orient.yaw;
			events.moteOrientation = rpy;
		}

		/*
		 *	If IR tracking is enabled then print the coordinates
		 *	on the virtual screen that the wiimote is pointing to.
		 *
		 *	Also make sure that we see at least 1 dot.
		 */
		if (WIIUSE_USING_IR(wm)) {
			int i = 0;

			/* go through each of the 4 possible IR sources */
			for (; i < 4; ++i) {
				/* check if the source is visible */
				if (wm->ir.dot[i].visible) {
					logVerbose("IR source %i: (%u, %u)", i, wm->ir.dot[i].x, wm->ir.dot[i].y);
				}
			}

			logVerbose("IR cursor: (%u, %u)", wm->ir.x, wm->ir.y);
			logVerbose("IR z distance: %f", wm->ir.z);

			//mManager.mWiimoteX = wm->ir.x;
			//mManager.mWiimoteY = wm->ir.y;
		}

		/* show events specific to supported expansions */
		if (wm->exp.type == EXP_NUNCHUK || wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
			/* nunchuk */
			struct nunchuk_t* nc = (nunchuk_t*)&wm->exp.nunchuk;

			if (IS_PRESSED(nc, NUNCHUK_BUTTON_C)) {
				logVerbose("Nunchuk: C pressed");
			}
			if (IS_PRESSED(nc, NUNCHUK_BUTTON_Z)) {
				logVerbose("Nunchuk: Z pressed");
			}

			logVerbose("nunchuk roll  = %f", nc->orient.roll);
			logVerbose("nunchuk pitch = %f", nc->orient.pitch);
			logVerbose("nunchuk yaw   = %f", nc->orient.yaw);

			Orientation rpy;
			rpy.pitch = nc->orient.pitch;
			rpy.roll = nc->orient.roll;
			rpy.yaw = nc->orient.yaw;
			events.chuckOrientation = rpy;

			logVerbose("nunchuk joystick angle:     %f", nc->js.ang);
			logVerbose("nunchuk joystick magnitude: %f", nc->js.mag);

			logVerbose("nunchuk joystick vals:      %f, %f", nc->js.x, nc->js.y);
			logVerbose("nunchuk joystick calibration (min, center, max): x: %i, %i, %i  y: %i, %i, %i",
				nc->js.min.x,
				nc->js.center.x,
				nc->js.max.x,
				nc->js.min.y,
				nc->js.center.y,
				nc->js.max.y);

			Joystick joy;
			joy.angle = nc->js.ang;
			joy.magni = nc->js.mag;
			joy.x = nc->js.x;
			joy.y = nc->js.y;
			events.chuckJoystick = joy;
		}
		else if (wm->exp.type == EXP_CLASSIC) {
			/* classic controller */
			struct classic_ctrl_t* cc = (classic_ctrl_t*)&wm->exp.classic;

			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_ZL)) {
				logVerbose("Classic: ZL pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_B)) {
				logVerbose("Classic: B pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_Y)) {
				logVerbose("Classic: Y pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_A)) {
				logVerbose("Classic: A pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_X)) {
				logVerbose("Classic: X pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_ZR)) {
				logVerbose("Classic: ZR pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_LEFT)) {
				logVerbose("Classic: LEFT pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_UP)) {
				logVerbose("Classic: UP pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_RIGHT)) {
				logVerbose("Classic: RIGHT pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_DOWN)) {
				logVerbose("Classic: DOWN pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_FULL_L)) {
				logVerbose("Classic: FULL L pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_MINUS)) {
				logVerbose("Classic: MINUS pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_HOME)) {
				logVerbose("Classic: HOME pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_PLUS)) {
				logVerbose("Classic: PLUS pressed");
			}
			if (IS_PRESSED(cc, CLASSIC_CTRL_BUTTON_FULL_R)) {
				logVerbose("Classic: FULL R pressed");
			}

			logVerbose("classic L button pressed:         %f", cc->l_shoulder);
			logVerbose("classic R button pressed:         %f", cc->r_shoulder);
			logVerbose("classic left joystick angle:      %f", cc->ljs.ang);
			logVerbose("classic left joystick magnitude:  %f", cc->ljs.mag);
			logVerbose("classic right joystick angle:     %f", cc->rjs.ang);
			logVerbose("classic right joystick magnitude: %f", cc->rjs.mag);
		}
		else if (wm->exp.type == EXP_GUITAR_HERO_3) {
			/* guitar hero 3 guitar */
			struct guitar_hero_3_t* gh3 = (guitar_hero_3_t*)&wm->exp.gh3;

			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_STRUM_UP)) {
				logVerbose("Guitar: Strum Up pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_STRUM_DOWN)) {
				logVerbose("Guitar: Strum Down pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_YELLOW)) {
				logVerbose("Guitar: Yellow pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_GREEN)) {
				logVerbose("Guitar: Green pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_BLUE)) {
				logVerbose("Guitar: Blue pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_RED)) {
				logVerbose("Guitar: Red pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_ORANGE)) {
				logVerbose("Guitar: Orange pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_PLUS)) {
				logVerbose("Guitar: Plus pressed");
			}
			if (IS_PRESSED(gh3, GUITAR_HERO_3_BUTTON_MINUS)) {
				logVerbose("Guitar: Minus pressed");
			}

			logVerbose("Guitar whammy bar:          %f", gh3->whammy_bar);
			logVerbose("Guitar joystick angle:      %f", gh3->js.ang);
			logVerbose("Guitar joystick magnitude:  %f", gh3->js.mag);
		}
		else if (wm->exp.type == EXP_WII_BOARD) {
			/* wii balance board */
			struct wii_board_t* wb = (wii_board_t*)&wm->exp.wb;
			float total = wb->tl + wb->tr + wb->bl + wb->br;
			float x = ((wb->tr + wb->br) / total) * 2 - 1;
			float y = ((wb->tl + wb->tr) / total) * 2 - 1;
			logVerbose("Weight: %f kg @ (%f, %f)", total, x, y);
			logVerbose("Interpolated weight: TL:%f  TR:%f  BL:%f  BR:%f", wb->tl, wb->tr, wb->bl, wb->br);
			logVerbose("Raw: TL:%d  TR:%d  BL:%d  BR:%d", wb->rtl, wb->rtr, wb->rbl, wb->rbr);

			BalanceBoard board;
			board.x = x;
			board.y = y;
			board.tr = wb->tr;
			board.tl = wb->tl;
			board.br = wb->br;
			board.bl = wb->bl;
			board.total = total;
			events.balanceBoard = board;
		}

		if (wm->exp.type == EXP_MOTION_PLUS ||
			wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
			logVerbose("Motion+ angular rates (deg/sec): pitch:%03.2f roll:%03.2f yaw:%03.2f",
				wm->exp.mp.angle_rate_gyro.pitch,
				wm->exp.mp.angle_rate_gyro.roll,
				wm->exp.mp.angle_rate_gyro.yaw);
		}

        mManager.mEventMutex.lock();
        mManager.mEvents[events.id - 1] = events;
        mManager.mEventMutex.unlock();
	}

	short any_wiimote_connected(wiimote** wm, int mWiimote) {
		int i;
		if (!wm) {
			return 0;
		}

		for (i = 0; i < mWiimote; i++) {
			if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i])) {
				return 1;
			}
		}

		return 0;
	}

	void run()
	{
        using namespace std::chrono_literals;

		//DBG("Starting worker thread...");
		mWiimotes = wiiuse_init(MAX_WIIMOTES);
		wiiuse_set_output(LOGLEVEL_DEBUG, stdout);

		int found = wiiuse_find(mWiimotes, MAX_WIIMOTES, 5);
		if (!found) {
			("No wiimotes found.");
			return;
		}

		int connected = wiiuse_connect(mWiimotes, MAX_WIIMOTES);
		if (connected) {
			logVerbose("Connected to %i wiimotes (of %i found).", connected, found);
		}
		else {
			logVerbose("Failed to connect to any wiimote.");
			return;
		}

		wiiuse_set_leds(mWiimotes[0], WIIMOTE_LED_1);
		wiiuse_set_leds(mWiimotes[1], WIIMOTE_LED_2);
		wiiuse_set_leds(mWiimotes[2], WIIMOTE_LED_3);
		wiiuse_set_leds(mWiimotes[3], WIIMOTE_LED_4);
		wiiuse_rumble(mWiimotes[0], 1);
		wiiuse_rumble(mWiimotes[1], 1);

        std::this_thread::sleep_for(200ms);

		wiiuse_rumble(mWiimotes[0], 0);
		wiiuse_rumble(mWiimotes[1], 0);

		while (any_wiimote_connected(mWiimotes, MAX_WIIMOTES)) {
			if (wiiuse_poll(mWiimotes, MAX_WIIMOTES)) {
				/*
				 *	This happens if something happened on any wiimote.
				 *	So go through each one and check if anything happened.
				 */
				int i = 0;
				for (; i < MAX_WIIMOTES; ++i) {
					switch (mWiimotes[i]->event) {
					case WIIUSE_EVENT:
						/* a generic event occurred */
						handle_event(mWiimotes[i]);
						break;

					case WIIUSE_STATUS:
						/* a status event occurred */
						handle_ctrl_status(mWiimotes[i]);
						break;

					case WIIUSE_DISCONNECT:
					case WIIUSE_UNEXPECTED_DISCONNECT:
						/* the wiimote disconnected */
						handle_disconnect(mWiimotes[i]);
						break;

					case WIIUSE_READ_DATA:
						/*
						 *	Data we requested to read was returned.
						 *	Take a look at wiimotes[i]->read_req
						 *	for the data.
						 */
						break;

					case WIIUSE_NUNCHUK_INSERTED:
						/*
						 *	a nunchuk was inserted
						 *	This is a good place to set any nunchuk specific
						 *	threshold values.  By default they are the same
						 *	as the wiimote.
						 */
						 /* wiiuse_set_nunchuk_orient_threshold((struct nunchuk_t*)&wiimotes[i]->exp.nunchuk, 90.0f); */
						 /* wiiuse_set_nunchuk_accel_threshold((struct nunchuk_t*)&wiimotes[i]->exp.nunchuk, 100); */
						logVerbose("Nunchuk inserted.");
						break;

					case WIIUSE_CLASSIC_CTRL_INSERTED:
						logVerbose("Classic controller inserted.");
						break;

					case WIIUSE_WII_BOARD_CTRL_INSERTED:
						logVerbose("Balance board controller inserted.");
						break;

					case WIIUSE_GUITAR_HERO_3_CTRL_INSERTED:
						/* some expansion was inserted */
						handle_ctrl_status(mWiimotes[i]);
						logVerbose("Guitar Hero 3 controller inserted.");
						break;

					case WIIUSE_MOTION_PLUS_ACTIVATED:
						logVerbose("Motion+ was activated");
						break;

					case WIIUSE_NUNCHUK_REMOVED:
					case WIIUSE_CLASSIC_CTRL_REMOVED:
					case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
					case WIIUSE_WII_BOARD_CTRL_REMOVED:
					case WIIUSE_MOTION_PLUS_REMOVED:
						/* some expansion was removed */
						handle_ctrl_status(mWiimotes[i]);
						logVerbose("An expansion was removed.");
						break;

					default:
						break;
					}
				}
			}
		}
	}

private:
	Manager& mManager;

	wiimote** mWiimotes = nullptr;
};


//==============================================================================
//
// Manager
//
//==============================================================================


Manager::Manager()
{
	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	//freopen("CONOUT$", "w", stderr);
}

Manager::~Manager()
{
	if (mWorkerThread.has_value()) {
		//DBG("Joining worker thread...");
		mWorkerThread->join();
	}
}

/*static*/ std::optional<int> Manager::buttonToWiimoteCode(MoteButton button)
{
	switch (button) {
	case MoteButton_Two:
		return WIIMOTE_BUTTON_TWO;
	case MoteButton_One:
		return WIIMOTE_BUTTON_ONE;
	case MoteButton_B:
		return WIIMOTE_BUTTON_B;
	case MoteButton_A:
		return WIIMOTE_BUTTON_A;
	case MoteButton_Minus:
		return WIIMOTE_BUTTON_MINUS;
	case MoteButton_Home:
		return WIIMOTE_BUTTON_HOME;
	case MoteButton_Left:
		return WIIMOTE_BUTTON_LEFT;
	case MoteButton_Right:
		return WIIMOTE_BUTTON_RIGHT;
	case MoteButton_Down:
		return WIIMOTE_BUTTON_DOWN;
	case MoteButton_Up:
		return WIIMOTE_BUTTON_UP;
	case MoteButton_Plus:
		return WIIMOTE_BUTTON_PLUS;
	default:
		break;
	}

	return std::nullopt;
}

void Manager::init()
{
	if (!mWorkerThread) {
		mWorker = std::make_unique<Worker>(*this);
		mWorkerThread = std::thread(&Worker::run, mWorker.get());
	}
}

void Manager::update()
{
    if (!mCallback)
        return;

    for (size_t i = 0; i < MAX_WIIMOTES; ++i) {
        if (mEvents[i].has_value()) {
            mCallback(mEvents[i].value());
            mEvents[i].reset();
        }
    }
}

} // namespace Wiimote
