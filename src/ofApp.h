#pragma once
#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxSerial.h"
#include "ofUtils.h"
#include "ofxImGui.h"
#include "./Theme.h"

#define USENDI
#define USESYPHON

#ifdef USENDI
#include "ofxNDIGrabber.h"
#endif

#ifdef USESYPHON
#include "ofxSyphon.h"
#endif

enum INPUTMODE {
    INPUTMODE_NDIGRABBER = 0,
    INPUTMODE_VIDEOPLAYER,
    INPUTMODE_VIDEOGRABBER,
    INPUTMODE_SYPHON
};

class ofApp : public ofBaseApp
{
public:
    ofApp(INPUTMODE mode, std::string mask);
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


    void stopPlayer();
    void togglePlayer();
    void nextVideo();
    void previousVideo();
    void saveCurrentRecording();
    void saveCurrentRecording(std::string path);
    void exportVideo(std::string path);

#ifdef USENDI
    ofxNDIGrabber _ndiGrabber;
    std::vector<ofVideoDevice> _ndiGrabberDevices;
#endif
#ifdef USESYPHON
    ofxSyphonClient _syphonClient;
    ofFbo _syphonFbo;
#endif
    ofVideoPlayer _videoPlayer;
    ofVideoPlayer _exportVideoPlayer;
    ofVideoGrabber _videoGrabber;
    std::vector<ofVideoDevice> _videoGrabberDevices;
    ofParameter<int> _selectedNdiDevice;
    ofParameter<int> _selectedVideoGrabberDevice;


    std::vector<ofPoint> _points;
    std::vector<ofColor> _oldLedPixels;
    std::vector<ofColor> _newLedPixels;
    
    ofParameter<bool> _recording;
    ofJson _recordedAnimation;
    uint64_t _recordingStartedTimestamp;

    ofxIO::PacketSerialDevice _serialDevice;

    ofParameterGroup _parameters;
    ofParameter<bool> _loop;
    ofParameter<bool> _mute;
    ofParameter<bool> _drawMask;

    std::vector<std::string> _modeLabels;
    ofParameter<int> _mode;

    std::vector<std::string> _videoFiles;
    int _selectedVideoIndex;

    ofxImGui::Gui _gui;
    std::vector<ofx::IO::SerialDeviceInfo> _serialDevices;
    std::vector<std::string> _serialDeviceLabels;
    ofParameter<std::string> _serialPort;

    void setupGui();
    void updateSerialDeviceList();
    void updateNDIGrabberList();
    void updateVideoGrabberList();

    void loadVideoByIndex(int index, bool loop);
    void loadVideoByPath(std::string path, bool loop);
    void addVideo(std::string path);
    void clearVideos();
    void sendSerial();

    
    // listeners
    void onSerialBuffer(const ofxIO::SerialBufferEventArgs &args);
    void onSerialError(const ofxIO::SerialBufferErrorEventArgs &args);

    void onModeChange(int & mode);
    void onMuteChange(bool & value);
    void onLoopChange(bool & value);
    void onRecordingChange(bool & value);
    
    void onNDIDeviceChange(int & value);
    void onVideoGrabberDeviceChange(int & value);
    void onSerialPortChange(std::string & value);
};
