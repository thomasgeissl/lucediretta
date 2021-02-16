#include "ofMain.h"
#include "ofApp.h"
#include "ofxCommandLineUtils.h"

int main(int argc, char *argv[]){
    cxxopts::Options options("luce diretta", "direct light");
    options.add_options()
    ("m,mode", "mode", cxxopts::value<std::string>()->default_value("NDIGRABBER"))
    ("M,mask", "mask", cxxopts::value<std::string>()->default_value(""));

    auto result = options.parse(argc, argv);
    auto modeString = result["mode"].as<std::string>();
    INPUTMODE mode;
    if(modeString == "NDIGRABBER"){
        mode = INPUTMODE::INPUTMODE_NDIGRABBER;
    }else if(modeString == "VIDEOPLAYER"){
        mode = INPUTMODE::INPUTMODE_VIDEOPLAYER;
    }else if(modeString == "VIDEOGRABBER"){
        mode = INPUTMODE::INPUTMODE_VIDEOGRABBER;
    }
    
    
    ofSetupOpenGL(1024/2,768/2,OF_WINDOW);
	ofRunApp(new ofApp(mode, result["mask"].as<std::string>()));
}
