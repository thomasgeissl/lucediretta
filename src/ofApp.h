#pragma once
#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxSerial.h"
#include "ofUtils.h"
#include "ofxImGui.h"
#include "./Theme.h"

#define USENDI

#ifdef USENDI
#include "ofxNDIGrabber.h"
#endif

enum INPUTMODE {
    INPUTMODE_NDIGRABBER = 0,
    INPUTMODE_VIDEOPLAYER,
    INPUTMODE_VIDEOGRABBER,
};

class ofApp : public ofBaseApp
{
public:
    ofApp();
    ~ofApp();
    void setup();
    void update();
    void draw();
    void exit();

    void loadMask(std::string path);
    void loadVideo(std::string path, bool loop);

    bool connectToArduino();
    bool connectToArduino(std::string description);
#ifdef USENDI
    bool connectToNDISender(std::string name);
#endif
    void keyReleased(int key);

    void dragEvent(ofDragInfo info);
    void mouseMoved(int x, int y);
    void mouseReleased(int x, int y, int button);

    ofJson _config;

// private:
#ifdef USENDI
    ofxNDIGrabber _ndiGrabber;
#endif
    ofVideoPlayer _videoPlayer;
    ofVideoGrabber _videoGrabber;
    std::vector<ofVideoDevice> _ndiDevices;
    int _selectedNdiDevice;


    bool _loopVideos;
    std::vector<ofPoint> _points;
    std::vector<ofColor> _oldLedPixels;
    std::vector<ofColor> _newLedPixels;

    ofxIO::PacketSerialDevice _device;

    ofParameterGroup _parameters;
    ofParameter<bool> _loopVideoParameter;
    ofParameter<bool> _drawMask;

    std::vector<std::string> _modeLabels;
    ofParameter<int> _mode;

    std::vector<std::string> _videoFiles;
    int _selectedVideoIndex;

    ofxImGui::Gui gui;
    std::vector<ofx::IO::SerialDeviceInfo> _serialDevices;
    int _selectedSerialDevice;

    void setupGui();
    void updateSerialDeviceList();

    void loadVideoByIndex(int index, bool loop);
    void loadVideoByPath(std::string path, bool loop);
    void addVideo(std::string path);
    void clearVideos();
    void sendSerial();

    void onSerialBuffer(const ofxIO::SerialBufferEventArgs &args);
    void onSerialError(const ofxIO::SerialBufferErrorEventArgs &args);
};