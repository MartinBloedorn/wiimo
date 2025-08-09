#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include <string>

#include "WiimoteManager.h"
#include "Output.h"

class ofApp : public ofBaseApp
{
    Wiimote::Manager mWiimoteManager;

    ofxPanel mGui;
    ofxButton button;

    ofxInputField<std::string> mGuiOscHost;
	ofxInputField<int> mGuiOscPort;
	ofxLabel mGuiOscState;

	WiimoOscOutput mOscOut;

public:
	void setup();
	void update();
	void draw();

	void guiOscHostChanged(std::string & host);
	void guiOscPortChanged(int & port);

	void handleOscSetup();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

    void onControllerEvents(const Wiimote::ControllerEvents& events);
};
