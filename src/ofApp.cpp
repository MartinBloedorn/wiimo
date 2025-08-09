#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{ 
    ofSetLogLevel(OF_LOG_VERBOSE);

    mGui.setup();

    //button.setup("Print Text");
    //button.addListener(this, &ofApp::onButtonPressed);
    //gui.add(&button);
	mGui.add(mGuiOscHost.setup("Host", "127.0.0.1"));
	mGui.add(mGuiOscPort.setup("Port", 12021, 1, 99999));
	mGui.add(mGuiOscState.setup("OSC", "disconnected"));

	mGuiOscHost.addListener(this, &ofApp::guiOscHostChanged);
	mGuiOscPort.addListener(this, &ofApp::guiOscPortChanged);
	
    mWiimoteManager.init();
    mWiimoteManager.onControllerEvents([this](const Wiimote::ControllerEvents& events) {
        this->onControllerEvents(events);

		mOscOut.processControllerEvents(events);
    });

	handleOscSetup();
}

//--------------------------------------------------------------
void ofApp::update()
{
    mWiimoteManager.update();
}

void ofApp::onControllerEvents(const Wiimote::ControllerEvents& events)
{
	for (int i = 0; i < Wiimote::MoteButtonEnd; ++i) {
        if (auto t = events.moteButtonTransitions[i]; t != Wiimote::TransitionNone) {
            ofLogNotice() << events.id << " BUTTON " << i << " :: " << (t == Wiimote::TransitionPressed ? "Pressed" : "Released");
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofBackground(50, 50, 50);
    mGui.draw();
}

void ofApp::guiOscHostChanged(std::string & host)
{
	handleOscSetup();
}

void ofApp::guiOscPortChanged(int & port)
{
	handleOscSetup();
}

void ofApp::handleOscSetup()
{
	bool r  = mOscOut.setup(mGuiOscHost, mGuiOscPort);
	mGuiOscState = r ? "connected" : "disconnected";
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
