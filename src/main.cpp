#include "ofMain.h"
#include "ofApp.h"
#include "ofxCommandLineUtils.h"

int main(){
	ofSetupOpenGL(1024/2,768/2,OF_WINDOW);
	ofRunApp(new ofApp());
}