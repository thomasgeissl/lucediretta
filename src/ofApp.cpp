#include "ofApp.h"
#include "../libs/IconFontCppHeaders/IconsFontAwesome5.h"

ofApp::ofApp() 
{
    ofSetWindowTitle("luce diretta");
    ofSetFrameRate(60);

    _videoPlayer.setLoopState(ofLoopType::OF_LOOP_NONE);
    loadMask("mask.svg");
    _drawMask.set("draw mask", false);

    ofSetBackgroundColor(ofColor(32,32,32));

#ifdef USENDI
    _ndiDevices = _ndiGrabber.listDevices();
    for (auto & device : _ndiGrabber.listDevices())
    {
        ofLogNotice("led animation toolkit") << device.id << " " << device.deviceName;
        connectToNDISender(device.deviceName);
    }
#endif

    _selectedVideoIndex = -1;

    ofFile file("config.json");
    if (file.exists())
    {
        file >> _config;
        ofLogNotice("led animation toolkit") << "successfully loaded config";
        ofLogNotice() << _config.dump(2);
    }

    _modeLabels.push_back("NDI");
    _modeLabels.push_back("video");


    updateSerialDeviceList();
    // connectToArduino();
}

ofApp::~ofApp()
{
    exit();
}

void ofApp::setup()
{
    // Set Window Size
    ofSetWindowShape(1024, 768);

    ofDirectory dir("videos");
    dir.allowExt("mov");
    dir.allowExt("mp4");
    dir.listDir();

    for (int i = 0; i < dir.size(); i++)
    {
        ofLogNotice() << ofFilePath::join(ofFilePath::getEnclosingDirectory(dir.getAbsolutePath()), dir.getPath(i));
        addVideo(ofFilePath::join(ofFilePath::getEnclosingDirectory(dir.getAbsolutePath()), dir.getPath(i)));
    }
    _videoPlayer.setPaused(true);
    _loopVideos = _config["loopVideos"];
    if (!_loopVideos)
    {
        _videoPlayer.setLoopState(OF_LOOP_NONE);
    }

    _mode.set("mode", 0, 0, _modeLabels.size());
    setupGui();
}

void ofApp::update()
{
    ofLogNotice() << "update";
    auto frameNumber = ofGetFrameNum();
    // if(frameNumber % 120 == 0){
    //     updateSerialDeviceList();
    // } 
    // _client.update();
    std::string message;
    bool isNewFrame = false;

    if (_mode == INPUTMODE::INPUTMODE_NDIGRABBER)
    {
#ifdef USENDI
        _ndiGrabber.update();
        if (_ndiGrabber.isFrameNew())
        {
            isNewFrame = true;
            auto pixels = _ndiGrabber.getPixels();
            if (pixels.isAllocated())
            {
                for (auto i = 0; i < _points.size(); i++)
                {
                    auto point = _points[i];
                    auto color = _ndiGrabber.getPixels().getColor(_ndiGrabber.getWidth() * point.x, _ndiGrabber.getHeight() * point.y);
                    int r = color.r;
                    int g = color.g;
                    int b = color.b;

                    _newLedPixels[i] = color;
                }
            }
        }
#endif
    }
    else if (_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER)
    {
        _videoPlayer.update();
        if (_videoPlayer.isFrameNew())
        {
            isNewFrame = true;
            for (auto i = 0; i < _points.size(); i++)
            {
                auto point = _points[i];
                auto color = _videoPlayer.getPixels().getColor(_videoPlayer.getWidth() * point.x, _videoPlayer.getHeight() * point.y);
                int r = color.r;
                int g = color.g;
                int b = color.b;

                _newLedPixels[i] = color;
            }
        }
    }

    if (isNewFrame)
    {
        sendSerial();
    }
}

void ofApp::draw()
{
    ofLogNotice() << "draw";
    ofPushMatrix();
    ofTranslate(532, 112);

    auto width = ofGetWidth() - 572;
    auto height = 0;

    int vX = 0;
    int vY = 0;

    if(_mode == INPUTMODE::INPUTMODE_NDIGRABBER)
    {
#ifdef USENDI
        height = (_ndiGrabber.getHeight() / _ndiGrabber.getWidth()) * width;
        _ndiGrabber.draw(0, 0, width, height);
#endif
    }
    else if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER)
    {
        height = (_videoPlayer.getHeight() / _videoPlayer.getWidth()) * width;
        _videoPlayer.draw(0, 0, width, height);
    }
    // cout << height << "\n" ;
    ofPopMatrix();

    // SVG Mask

    // if (_drawMask)
    {
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(532, 112);

        for (auto i = 0; i < _points.size(); i++)
        {
            auto point = _points[i];
            ofSetColor(0, 255, 0);
            ofDrawRectangle(point.x * width, point.y * height, 2, 2);
            //            ofSetColor(255, 255, 255);
            //            ofDrawBitmapString(ofToString(i), point.x*width, point.y*height);
        }
        ofPopMatrix();
        ofPopStyle();
    }
    //    ofLogNotice("default Loop: ") << _config["defaultLoop"].is_null();
    // Play default video when there is no other video playing
    if (_videoPlayer.getIsMovieDone() && _config["defaultLoop"].is_null() == false && _config["defaultLoop"]["path"].is_null() == false)
    {
        loadVideoByPath(_config["defaultLoop"]["path"], true);
    }

    //    ofLogNotice("movie is done: ") << (_videoPlayer.getIsMovieDone() ? "true" : "false");
    //    ofLogNotice("movie is playing: ") << (_videoPlayer.isPlaying() ? "true" : "false");


        gui.begin();
        auto settings = ofxImGui::Settings();
        auto offset = 10;
        ImGui::SetNextWindowPos(glm::vec2(offset, offset)); 
        ImGui::SetNextWindowSize(glm::vec2(ofGetWidth() - 2*offset, 0)); 
        if (ImGui::Begin("I/O")){
            if (ImGui::BeginCombo("mode", _modeLabels[_mode].c_str()))
            {
                for (auto i = 0; i < _modeLabels.size(); i++)
                {
                    bool is_selected = (_mode == i); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(_modeLabels[i].c_str(), is_selected))
                        _mode = i;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
            // ImGui::SameLine();
            if(_mode == INPUTMODE::INPUTMODE_NDIGRABBER && !_ndiDevices.empty()){
                if (ImGui::BeginCombo("ndi device", _ndiDevices[_selectedNdiDevice].deviceName.c_str()))
                {
                    for (auto i = 0; i < _ndiDevices.size(); i++)
                    {
                        bool is_selected = (_selectedNdiDevice == i); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(_ndiDevices[i].deviceName.c_str()), is_selected)
                            _selectedNdiDevice = i;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                    }
                    ImGui::EndCombo();
                }
            }
            // ImGui::SameLine();

            if (!_serialDevices.empty() && ImGui::BeginCombo("serial", _serialDevices[_selectedSerialDevice].getPort().c_str()))
            {
                for (auto i = 0; i < _serialDevices.size(); i++)
                {
                    bool is_selected = (_selectedSerialDevice == i); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(_serialDevices[i].getPort().c_str(), is_selected))
                        _selectedSerialDevice = i;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }

            // ImGui::SameLine();
            // ofxImGui::AddParameter(_drawMask);
            ImGui::End();
        }



        if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER){
            // ImGui::SetNextWindowPos(glm::vec2(offset, offset*10)); 
            ImGui::SetNextWindowSize(glm::vec2(ofGetWidth()/4 - 2*offset, 0)); 
            if (ImGui::Begin("library")){
                if(ofxImGui::VectorListBox("", &_selectedVideoIndex, _videoFiles)){
                    // ofLogNotice("TODO") << "trigger video " << _selectedVideoIndex;
                }
            }
            ImGui::End();
        }
    gui.end();
}

void ofApp::exit()
{

#ifdef NDI_ENABLED
//    _ndiGrabber.close();
#endif
    // if (_serial.isOpen())
    // {
    //     ofx::IO::ByteBuffer textBuffer(message);
    //     _serial.writeBytes(textBuffer);
    //     _serial.writeByte('\n');
    // }

    for (auto i = 0; i < _points.size(); i++)
    {

        try
        {
            // attention: wild cast here, assumption x < 256
            std::vector<uint8_t> data = {(unsigned char)(i), 0, 0, 0};
            ofx::IO::ByteBuffer buffer(data);
            _device.send(buffer);
        }
        catch (...)
        {
        }
    }
}

void ofApp::loadMask(std::string path)
{
    ofFile file(path);
    if (ofToLower(file.getExtension()) != "svg")
    {
        ofLogError("led animation toolkit") << "could not load mask. file format not supported (yet)";
        return;
    }
    std::vector<ofPoint> points;
    ofxXmlSettings mask;
    mask.load(path);
    auto viewBoxString = mask.getAttribute("svg", "viewBox", "");
    auto viewBox = ofSplitString(viewBoxString, " ");
    mask.pushTag("svg");
    for (auto i = 0; i < mask.getNumTags("rect"); i++)
    {
        if (viewBox.size() >= 3)
        {
            float x = mask.getAttribute("rect", "x", -1, i);
            float y = mask.getAttribute("rect", "y", -1, i);
            points.push_back(ofPoint(x / ofToInt(viewBox[2]), y / ofToInt(viewBox[3])));
            cout << viewBox[2] << " : " << viewBox[3];
        }
    }
    mask.popTag();
    _points = points;
    _oldLedPixels.resize(_points.size());
    _newLedPixels.resize(_points.size());

    //    _newLedPixels.allocate(_points.size(), 1, 3);
    //    _oldLedPixels.allocate(_points.size(), 1, 3);

    ofLogNotice() << "successfully loaded mask, number of points: " << _points.size();
}
void ofApp::loadVideo(std::string path, bool loop)
{
    ofLogNotice() << "load video" << path;
    ofFile file(path);
    if (ofToLower(file.getExtension()) != "mp4" && ofToLower(file.getExtension()) != "mov")
    {
        ofLogError() << "could not load video. file format not supported (yet)";
        return;
    }
    // _videoPlayer.load(path);
    _videoPlayer.play();
    if (!loop)
    {
        _videoPlayer.setLoopState(OF_LOOP_NONE);
    }
    ofLogNotice() << "successfully loaded video";
}

void ofApp::loadVideoByIndex(int index, bool loop)
{
    if (index == -1)
    {
        index = _videoFiles.size() - 1;
    }

    if (index >= _videoFiles.size())
    {
        index = 0;
    }

    loadVideo(_videoFiles[index], loop);
    _selectedVideoIndex = index;
}
void ofApp::loadVideoByPath(std::string path, bool loop)
{
    auto index = 0;
    ofLogNotice() << "path " << path << ", data path " << ofFilePath::getAbsolutePath(path);
    for (auto file : _videoFiles)
    {
        ofLogNotice() << "comparing against path " << file;

        if (ofFilePath::getAbsolutePath(path) == file)
        {
            ofLogNotice() << "found video";
            loadVideo(file, loop);
            _selectedVideoIndex = index;
            return;
        }
        index++;
    }
    ofLogNotice() << "no video found";
}

bool ofApp::connectToArduino()
{
    std::vector<ofx::IO::SerialDeviceInfo> devicesInfo = ofx::IO::SerialDeviceUtils::listDevices();
    for (auto & device : devicesInfo)
    {
        if ((ofToLower(device.getDescription()).find("arduino") != std::string::npos) || (ofToLower(device.getDescription()).find("usb serial device") != std::string::npos) || (ofToLower(device.getDescription()).find("teensyduino") != std::string::npos))
        {
            _device.setup(device.getPort(), 115200);
        }
    }
    auto success = _device.isOpen();
    if (success)
    {
        ofLogNotice("led animation toolkit") << "successfully opened serial connection";
    }
    else
    {
        ofLogError("led animation toolkit") << "could not connect to serial device";
    }
    return success;
}

bool ofApp::connectToArduino(std::string description)
{
    std::vector<ofx::IO::SerialDeviceInfo> devicesInfo = ofx::IO::SerialDeviceUtils::listDevices();
    for (auto device : devicesInfo)
    {
        if (ofToLower(device.getDescription()) == ofToLower(description))
        {
            auto success = _device.setup(devicesInfo[0], 115200);

            if (success)
            {
                _device.registerAllEvents(this);
                ofLogNotice("ofApp::setup") << "Successfully setup " << devicesInfo[0];
            }
            else
            {
                ofLogNotice("ofApp::setup") << "Unable to setup " << devicesInfo[0];
            }
        }
    }
    auto success = _device.setup(devicesInfo[0], 115200);

    if (success)
    {
        _device.registerAllEvents(this);
        ofLogNotice("ofApp::setup") << "Successfully setup " << devicesInfo[0];
    }
    else
    {
        ofLogNotice("ofApp::setup") << "Unable to setup " << devicesInfo[0];
    }

    return success;
}
#ifdef USENDI
bool ofApp::connectToNDISender(std::string name)
{
    // Reset current video index
    _selectedVideoIndex = -1;

    auto success = true;
    _ndiGrabber.setDevice(name);
    return success;
}
#endif

void ofApp::keyReleased(int key)
{
    switch (key)
    {
    case OF_KEY_RETURN:
    case ' ':
    {
        if (_videoPlayer.isPlaying())
        {
            _videoPlayer.setPaused(true);
        }
        else
        {
            _videoPlayer.play();
        }
        break;
    }
    case 'c':
    {
        connectToArduino();
        break;
    }
    case 'm':
    {
        ofFileDialogResult openFileResult = ofSystemLoadDialog("select mask");
        if (openFileResult.bSuccess)
        {
            loadMask(openFileResult.getPath());
        }
        break;
    }
    case 'v':
    {
        ofFileDialogResult openFileResult = ofSystemLoadDialog("select video");
        if (openFileResult.bSuccess)
        {
            loadVideo(openFileResult.getPath(), _loopVideos);
        }
        break;
    }
    case OF_KEY_UP:
    case OF_KEY_LEFT:
    {
        loadVideoByIndex(_selectedVideoIndex - 1, _loopVideos);
        break;
    }
    case OF_KEY_DOWN:
    case OF_KEY_RIGHT:
    {
        loadVideoByIndex(_selectedVideoIndex + 1, _loopVideos);
        break;
    }
    }

    // Play Video by number
    int keyNumber = key - '0';
    if (keyNumber >= 1 && keyNumber <= 9)
    {
        if (keyNumber <= _videoFiles.size())
        {
            loadVideoByIndex(keyNumber - 1, _loopVideos);
        }
    }
}

void ofApp::dragEvent(ofDragInfo info)
{
    // ofLogVerbose("DLR 2.0")<<info.files.back();

    std::vector<std::string> files;

    ofFile ofFiles(info.files.back());
    if (ofFiles.isDirectory())
    {

        ofDirectory dir(info.files.back() + "/");

        for (boost::filesystem::directory_entry &p : std::filesystem::directory_iterator(info.files.back()))
        {
            std::cout << p.path().generic_string() << '\n';
            files.push_back(p.path().generic_string());
        }
    }
    else
    {
        files = info.files;
    }

    for (std::string &filePath : files)
    {
        ofFile file(filePath);
        if (ofToLower(file.getExtension()) == "mov" || ofToLower(file.getExtension()) == "mp4")
        {
            addVideo(filePath);
            // _mode = INPUTMODE::INPUTMODE_VIDEOPLAYER;
        }
        else if (ofToLower(file.getExtension()) == "svg")
        {
            loadMask(info.files.back());
        }
    }
}

void ofApp::addVideo(std::string path)
{
    _videoFiles.push_back(path);
    // loadVideoByIndex(0, true);
}

void ofApp::clearVideos()
{
    _videoPlayer.stop();
    _videoFiles.clear();
    _selectedVideoIndex = -1;
}

void ofApp::updateSerialDeviceList()
{
    _serialDevices = ofx::IO::SerialDeviceUtils::listDevices();
}

void ofApp::setupGui()
{
    ImGui::CreateContext();  
    ImGui::GetIO().Fonts->AddFontFromFileTTF(&ofToDataPath("fonts/Roboto-Light.ttf")[0], 16.f);
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    // ImGui::GetIO().Fonts->AddFontFromFileTTF(&ofToDataPath("fonts/fa-regular-400.ttf")[0], 16.0f, &icons_config, icons_ranges );
    gui.setup();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().MouseDrawCursor = false;

    gui.setTheme(new Theme());
}


void ofApp::sendSerial()
{
    ofLogNotice() << "send serial";
    for (auto x = 0; x < _newLedPixels.size(); x++)
    {
        auto oldColor = _oldLedPixels[x];
        auto newColor = _newLedPixels[x];

        if (newColor.r != oldColor.r || newColor.g != oldColor.g || newColor.b != oldColor.b)
        {
            try
            {
                // attention: wild cast here, assumption x < 256
                std::vector<uint8_t> data = {(unsigned char)(x), newColor.r, newColor.g, newColor.b};
                ofx::IO::ByteBuffer buffer(data);
                _device.send(buffer);
            }
            catch (...)
            {
            }
        }
    }
    _oldLedPixels = _newLedPixels;
}

void ofApp::mouseMoved(int x, int y)
{
}

void ofApp::mouseReleased(int x, int y, int button)
{
}

void ofApp::onSerialBuffer(const ofx::IO::SerialBufferEventArgs &args)
{
    ofLogNotice() << "Serial message " << args.buffer().toString();
}

void ofApp::onSerialError(const ofx::IO::SerialBufferErrorEventArgs &args)
{
    ofLogError() << "Serial error " << args.buffer().toString();
}
