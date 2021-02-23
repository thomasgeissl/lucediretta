#include "ofApp.h"
#include "../libs/IconFontCppHeaders/IconsFontAwesome5.h"

ofApp::ofApp(INPUTMODE mode, std::string mask)
{
    ofSetWindowTitle("luce diretta");
    ofSetFrameRate(60);
    ofSetBackgroundColor(ofColor(32,32,32));

    
#ifdef USENDI
    _ndiGrabberDevices = _ndiGrabber.listDevices();
    for (auto & device : _ndiGrabber.listDevices())
    {
        ofLogNotice() << device.id << " " << device.deviceName;
        connectToNDISender(device.deviceName);
    }
#endif
#ifdef USESYPHON
    _syphonClient.setup();
#endif

    if(!mask.empty()){
        if(ofFilePath::isAbsolute(mask)){
            loadMask(mask);
        }else{
            ofLogError() << mask <<" is not an absolute path, TODO: support relative file paths";
        }
        
    }else{
        loadMask("mask.svg");
    }
    _drawMask.set("draw mask", true);

    _mode.addListener(this, &ofApp::onModeChange);
    _mode.set("mode", mode, 0, _modeLabels.size());
    _mute.addListener(this, &ofApp::onMuteChange);
    _mute.set("mute", false);
    _loop.addListener(this, &ofApp::onLoopChange);
    _loop.set("loop", false);
    _recording.addListener(this, &ofApp::onRecordingChange);
    _recording.set("recording", false);
    _selectedNdiDevice.addListener(this, &ofApp::onNDIDeviceChange);
    _selectedNdiDevice.set("ndiDevice", 0);
    _selectedVideoGrabberDevice.addListener(this, &ofApp::onVideoGrabberDeviceChange);
    _selectedVideoGrabberDevice.set("videoGrabberDevice", 0);
    
    _serialPort.addListener(this, &ofApp::onSerialPortChange);
    _serialPort.set("serialPort", "");
    


    _selectedVideoIndex = -1;

    _modeLabels.push_back("NDI grabber");
    _modeLabels.push_back("video player");
    _modeLabels.push_back("video grabber");
    _modeLabels.push_back("syphon");


    updateSerialDeviceList();
    // connectToArduino();
}

ofApp::~ofApp()
{
    exit();
}

void ofApp::setup()
{
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

    setupGui();
    connectToArduino();
    updateVideoGrabberList();
}

void ofApp::update()
{
    auto frameNumber = ofGetFrameNum();
    if(frameNumber % 600 == 0){
//        updateSerialDeviceList();
//        updateVideoGrabberList();
    }
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
        ofLogNotice() << _videoPlayer.getDuration();
        ofLogNotice() << _videoPlayer.getPosition();
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
        
        if(_recording && _videoPlayer.getIsMovieDone()){
            _recording = false;
            saveCurrentRecording();
        }
    }
    else if (_mode == INPUTMODE::INPUTMODE_VIDEOGRABBER)
    {
         _videoGrabber.update();
         if (_videoGrabber.isFrameNew())
         {
             isNewFrame = true;
             for (auto i = 0; i < _points.size(); i++)
             {
                 auto point = _points[i];
                 auto color = _videoGrabber.getPixels().getColor(_videoGrabber.getWidth() * point.x, _videoGrabber.getHeight() * point.y);
                 int r = color.r;
                 int g = color.g;
                 int b = color.b;

                 _newLedPixels[i] = color;
             }
         }
    }
    else if (_mode == INPUTMODE::INPUTMODE_SYPHON)
    {
#ifdef USESYPHON
        if(!_syphonFbo.isAllocated()
           || _syphonClient.getWidth() != _syphonFbo.getWidth()
           || _syphonClient.getHeight() != _syphonFbo.getHeight()
           ){
            _syphonFbo.allocate(_syphonClient.getWidth(), _syphonClient.getHeight());
        }

        if(_syphonFbo.getWidth() == 0) return;
        _syphonFbo.begin();
        _syphonClient.draw(0,0);
        _syphonFbo.end();
        
        ofPixels pix;
        _syphonFbo.readToPixels(pix);
        isNewFrame = true;

        for (auto i = 0; i < _points.size(); i++)
        {
            auto point = _points[i];
            auto color = pix.getColor(_syphonClient.getWidth() * point.x, _syphonClient.getHeight() * point.y);
            int r = color.r;
            int g = color.g;
            int b = color.b;

            _newLedPixels[i] = color;
        }
#endif
    }



    if (isNewFrame)
    {
        sendSerial();
    }
}

void ofApp::draw()
{
    auto padding = ofGetWidth()*0.01;

    _gui.begin();
    auto settings = ofxImGui::Settings();
    auto settingsWindowPosition = glm::vec2(padding, padding);
    auto settingsWindowSize = glm::vec2(0, 0);

    ImGui::SetNextWindowPos(settingsWindowPosition); 
    ImGui::SetNextWindowSize(glm::vec2(ofGetWidth() - 2*padding, 0)); 

        if (ImGui::Begin("settings")){
            ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth()/2);

            if (ImGui::BeginCombo("mode", _modeLabels[_mode].c_str()))
            {
                for (auto i = 0; i < _modeLabels.size(); i++)
                {
                    bool is_selected = (_mode == i);
                    if (ImGui::Selectable(_modeLabels[i].c_str(), is_selected))
                        _mode = i;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
//            ImGui::SameLine();
            if(_mode == INPUTMODE::INPUTMODE_NDIGRABBER && !_ndiGrabberDevices.empty()){
                if (ImGui::BeginCombo("ndi device", _ndiGrabberDevices[_selectedNdiDevice].deviceName.c_str()))
                {
                    for (auto i = 0; i < _ndiGrabberDevices.size(); i++)
                    {
                        bool is_selected = (_selectedNdiDevice == i);
                        if (ImGui::Selectable(_ndiGrabberDevices[i].deviceName.c_str()), is_selected)
                            _selectedNdiDevice = i;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            if(_mode == INPUTMODE::INPUTMODE_VIDEOGRABBER && !_ndiGrabberDevices.empty()){
                std::string text = "not selected";
                if(_selectedVideoGrabberDevice >=  0 && _selectedVideoGrabberDevice < _videoGrabberDevices.size()){
                    text = _videoGrabberDevices[_selectedVideoGrabberDevice].deviceName;
                }
                if (ImGui::BeginCombo("video grabber", text.c_str()))
                {
                    for (auto i = 0; i < _videoGrabberDevices.size(); i++)
                    {
                        bool is_selected = (_selectedVideoGrabberDevice == i);
                        if (ImGui::Selectable(_videoGrabberDevices[i].deviceName.c_str()), is_selected)
                            _selectedVideoGrabberDevice = i;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            // ImGui::SameLine();

            if (!_serialDevices.empty() && ImGui::BeginCombo("serial port", _serialPort.get().c_str()))
            {
                for(auto & port : _serialDeviceLabels){
                    auto isSelected = port == _serialPort.get();
                    if (ImGui::Selectable(port.c_str(), isSelected)){
                        _serialPort = port;
                    }
                    if (isSelected){
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            if(ImGui::Button("load mask"))
            {
                ofFileDialogResult result = ofSystemLoadDialog("load svg mask", true);
                if(result.bSuccess) {
                    std::string path = result.getPath();
                    auto basename = ofFilePath::getBaseName(path);
                    auto extension = ofFilePath::getFileExt(path);
                    if(extension == "svg"){
                        loadMask(path);
                    }
                }
            }
            ImGui::SameLine();
            bool drawMaskValue = _drawMask.get();
            if(ImGui::Checkbox("draw mask", &drawMaskValue)){
                _drawMask = drawMaskValue;
            }
            
            settingsWindowSize = ImGui::GetWindowSize();
            ImGui::End();
        }

 ofPushMatrix();

    auto previewPosition = glm::vec2(padding, settingsWindowPosition.y + settingsWindowSize.y + padding);
    auto previewSize = glm::vec2(ofGetWidth()/2 - 2*padding, 0);
    ofTranslate(previewPosition.x, previewPosition.y);
    if(_mode == INPUTMODE::INPUTMODE_NDIGRABBER)
    {
#ifdef USENDI
        previewSize.y = (_ndiGrabber.getHeight() / _ndiGrabber.getWidth()) * previewSize.x;
        _ndiGrabber.draw(0, 0, previewSize.x, previewSize.y);
#endif
    }
    else if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER)
    {
        previewSize.y = (_videoPlayer.getHeight() / _videoPlayer.getWidth()) * previewSize.x;
        _videoPlayer.draw(0, 0, previewSize.x, previewSize.y);
        auto playheadHeight = 5;
        ofDrawRectangle(0, previewSize.y, _videoPlayer.getPosition()*previewSize.x, playheadHeight);
    }
    else if(_mode == INPUTMODE::INPUTMODE_VIDEOGRABBER)
    {
        previewSize.y = (_videoGrabber.getHeight() / _videoGrabber.getWidth()) * previewSize.x;
        _videoGrabber.draw(0, 0, previewSize.x, previewSize.y);
    }
    if(_mode == INPUTMODE::INPUTMODE_SYPHON)
    {
#ifdef USESYPHON
        previewSize.y = (_syphonClient.getHeight() / _syphonClient.getWidth()) * previewSize.x;
        _syphonClient.draw(0, 0, previewSize.x, previewSize.y);
#endif
    }
    ofPopMatrix();

    // SVG Mask

    if (_drawMask)
    {
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(padding, settingsWindowSize.y + padding);

        for (auto i = 0; i < _points.size(); i++)
        {
            auto point = _points[i];
            ofSetColor(0, 255, 0);
            ofDrawRectangle(point.x * previewSize.x, point.y * previewSize.y, 2, 2);
            //            ofSetColor(255, 255, 255);
            //            ofDrawBitmapString(ofToString(i), point.x*width, point.y*height);
        }
        ofPopMatrix();
        ofPopStyle();
    }

// ##### draw library
    auto libraryWindowPosition = glm::vec2(ofGetWidth()/2 + padding, settingsWindowPosition.y + settingsWindowSize.y + padding);
    auto libraryWindowSize = glm::vec2(ofGetWidth()/2 - 2*padding, previewSize.y);

        if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER){
            ImGui::SetNextWindowPos(libraryWindowPosition);
            ImGui::SetNextWindowSize(libraryWindowSize);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
            if (ImGui::Begin("library", NULL, flags)){
                std::vector<std::string> videoLabels;
                for(auto & file : _videoFiles){
                    videoLabels.push_back(ofFilePath::getBaseName(file));
                }
                ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
                if(ofxImGui::VectorListBox("", &_selectedVideoIndex, videoLabels)){
                    loadVideoByIndex(_selectedVideoIndex, false);
                }
                ImGui::PopItemWidth();
                
                if(ImGui::Button("add"))
                {
                    ofFileDialogResult result = ofSystemLoadDialog("add video", true);
                    if(result.bSuccess) {
                        std::string path = result.getPath();
                        auto basename = ofFilePath::getBaseName(path);
                        auto extension = ofFilePath::getFileExt(path);
                        if(ofDirectory::doesDirectoryExist(path)){
                            ofDirectory dir(path);
                            dir.listDir();
                            for(auto file : dir.getFiles()){
                                auto path = file.getAbsolutePath();
                                auto basename = ofFilePath::getBaseName(path);
                                auto extension = ofFilePath::getFileExt(path);
                                if(extension == "mp4" || extension == "mov"){
                                    addVideo(path);
                                }
                            }
                            
                        }else{
                            if(extension == "mp4" || extension == "mov"){
                                addVideo(path);
                            }
                        }

                    }
                }
            }
//            libraryWindowSize = ImGui::GetWindowSize();
            ImGui::End();
        }

// ##### draw player controls
    auto playerControlsWindowSize = glm::vec2(0,0);
    auto playerControlsWindowPosition = glm::vec2(previewPosition.x,  previewPosition.y + previewSize.y + padding);
        if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER){
            ImGui::SetNextWindowPos(playerControlsWindowPosition);
            ImGui::SetNextWindowSize(glm::vec2(previewSize.x, 0)); 
            if (ImGui::Begin("controls")){
                if(ImGui::Button("stop"))
                {
                    stopPlayer();
                }
                ImGui::SameLine();
                if(ImGui::Button("previous"))
                {
                    previousVideo();
                }
                ImGui::SameLine();
                if(!_videoPlayer.isPlaying()){
                    if(ImGui::Button("play"))
                    {
                        _videoPlayer.setPaused(false);
                        _videoPlayer.play();
                    }
                    ImGui::SameLine();
                }else{
                    if(ImGui::Button("pause"))
                    {
                        _videoPlayer.setPaused(true);
                    }
                    ImGui::SameLine();
                }
                if(ImGui::Button("next"))
                {
                    nextVideo();
                }
                ImGui::SameLine();
                 if(ImGui::Button("export"))
                 {
                    _videoPlayer.stop();
                    _recording = true;
                    _loop = false;
                    _videoPlayer.play();
                 }
                 ImGui::SameLine();
                bool loopValue = _loop.get();
                if(ImGui::Checkbox("loop", &loopValue)){
                    _loop = loopValue;
                }
                ImGui::SameLine();
                bool muteValue = _mute.get();
                if(ImGui::Checkbox("mute", &muteValue)){
                    _mute = muteValue;
                }
            }
            playerControlsWindowSize = ImGui::GetWindowSize();
            ImGui::End();
        }
    
    // ##### recorder controls
            if(_mode == INPUTMODE::INPUTMODE_VIDEOPLAYER){
                ImGui::SetNextWindowPos(glm::vec2(libraryWindowPosition.x,  playerControlsWindowPosition.y));
            }else{
                ImGui::SetNextWindowPos(glm::vec2(libraryWindowPosition.x,  previewPosition.y));
            }
    // + previewSize.y + 10 + padding)
                ImGui::SetNextWindowSize(glm::vec2(previewSize.x, 0));
                if (ImGui::Begin("recorder")){
                    bool recordingValue = _recording.get();
                    if(ImGui::Checkbox("recording", &recordingValue)){
                        _recording = recordingValue;
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("save")){
                        saveCurrentRecording();
                    }
                }
                ImGui::End();
            


//    ImGui::ShowStyleEditor();
    _gui.end();
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
            _serialDevice.send(buffer);
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
        ofLogError() << "could not load mask. file format not supported (yet)";
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
    _videoPlayer.load(path);
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

    if(index >= 0 && index < _videoFiles.size()){
        loadVideo(_videoFiles[index], loop);
        _selectedVideoIndex = index;
    }
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
            _serialPort = device.port();
        }
    }
    auto success = _serialDevice.isOpen();
    if (success)
    {
        ofLogNotice("") << "successfully opened serial connection";
    }
    else
    {
        ofLogError("") << "could not connect to serial device";
    }
    return success;
}

bool ofApp::connectToArduino(std::string description)
{
    std::vector<ofx::IO::SerialDeviceInfo> devicesInfo = ofx::IO::SerialDeviceUtils::listDevices();
    for (auto device : devicesInfo)
    {
        if (ofToLower(device.description()) == ofToLower(description))
        {
            auto success = _serialDevice.setup(devicesInfo[0], 115200);

            if (success)
            {
                _serialDevice.registerAllEvents(this);
                ofLogNotice() << "Successfully setup " << devicesInfo[0];
            }
            else
            {
                ofLogNotice() << "Unable to setup " << devicesInfo[0];
            }
        }
    }
    auto success = _serialDevice.setup(devicesInfo[0], 115200);

    if (success)
    {
        _serialDevice.registerAllEvents(this);
        ofLogNotice("") << "Successfully setup " << devicesInfo[0];
    }
    else
    {
        ofLogNotice("") << "Unable to setup " << devicesInfo[0];
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
        togglePlayer();
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
    case 'r':
    {
        _recording = !_recording;
        break;
    }
    case 'v':
    {
        ofFileDialogResult openFileResult = ofSystemLoadDialog("select video");
        if (openFileResult.bSuccess)
        {
            loadVideo(openFileResult.getPath(), _loop);
        }
        break;
    }
    case OF_KEY_UP:
    case OF_KEY_LEFT:
    {
        previousVideo();
        break;
    }
    case OF_KEY_DOWN:
    case OF_KEY_RIGHT:
    {
        nextVideo();
        break;
    }
    }

    // Play Video by number
    int keyNumber = key - '0';
    if (keyNumber >= 1 && keyNumber <= 9)
    {
        if (keyNumber <= _videoFiles.size())
        {
            loadVideoByIndex(keyNumber - 1, _loop);
        }
    }
}

void ofApp::dragEvent(ofDragInfo info)
{
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
             _mode = INPUTMODE::INPUTMODE_VIDEOPLAYER;
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
    _serialDeviceLabels.clear();
    for(auto & device : _serialDevices){
        _serialDeviceLabels.push_back(device.getPort());
    }
}

void ofApp::updateNDIGrabberList()
{
    _ndiGrabberDevices = _ndiGrabber.listDevices();
}

void ofApp::updateVideoGrabberList()
{
    _videoGrabberDevices = _videoGrabber.listDevices();
}

void ofApp::setupGui()
{
    _gui.setup();
    ImGui::CreateContext();
    auto io = ImGui::GetIO();
    auto font = io.Fonts->AddFontDefault();

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; // Will not be copied by AddFont* so keep in scope.
    ImFontConfig config;
//    config.MergeMode = true;
//    config.GlyphMinAdvanceX = 16.0f; // Use if you want to make the icon monospaced

//    ImGui::GetIO().Fonts->AddFontFromFileTTF(&ofToDataPath("fonts/Roboto-Light.ttf")[0], 16.f, &config, io.Fonts->GetGlyphRangesDefault());
//    ImGui::GetIO().Fonts->AddFontFromFileTTF(&ofToDataPath("fonts/fa-regular-400.ttf")[0], 16.0f, &config, icons_ranges );

    // ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Build atlas
//    io.Fonts->Build();

    // Retrieve texture in RGBA format
//    unsigned char* tex_pixels = NULL;
//    int tex_width, tex_height;
//    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

    
    
    ImGui::GetIO().MouseDrawCursor = false;

    _gui.setTheme(new Theme());
}


void ofApp::sendSerial()
{
    auto object = ofJson::object();
    if(_recording){
        object["time"] = ofGetElapsedTimeMillis() - _recordingStartedTimestamp;
        object["pixels"] = ofJson::array();
    }
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
                _serialDevice.send(buffer);
                if(_recording){
                    auto pixel = ofJson::object();
                    pixel["index"] = x;
                    pixel["r"] = newColor.r;
                    pixel["g"] = newColor.g;
                    pixel["b"] = newColor.b;
                    object["pixels"].push_back(pixel);
                }
            }
            catch (...)
            {
            }
        }
    }
    _oldLedPixels = _newLedPixels;
    
    if(_recording){
        _recordedAnimation.push_back(object);
    }
}

void ofApp::mouseMoved(int x, int y)
{
}

void ofApp::mouseReleased(int x, int y, int button)
{
}

void ofApp::stopPlayer(){
    _videoPlayer.stop();
    _selectedVideoIndex = -1;
}
void ofApp::togglePlayer(){
    if (_videoPlayer.isPlaying())
    {
        _videoPlayer.setPaused(true);
    }
    else
    {
        _videoPlayer.play();
    }
}
void ofApp::nextVideo(){
    loadVideoByIndex(_selectedVideoIndex + 1, _loop);
}
void ofApp::previousVideo(){
    loadVideoByIndex(_selectedVideoIndex - 1, _loop);
}

void ofApp::saveCurrentRecording(){
    auto result = ofSystemSaveDialog("luce.h", "choose destination");
    if(result.bSuccess) {
      std::string path = result.getPath();
        auto basename = ofFilePath::getBaseName(path);
        auto extension = ofFilePath::getFileExt(path);
//        if(extension == ".json"){
            saveCurrentRecording(path);
//        }
    }
}
void ofApp::saveCurrentRecording(std::string path){
    ofLogNotice() << "save recording to " << path;
    auto name = ofFilePath::getBaseName(path);
    ofLogNotice() << name;

    ofFile output;
    output.create();
    output.open(path, ofFile::WriteOnly);
//    output << _recordedAnimation.dump();
    

    


    std::string createFunctionCode = "void create_";
    createFunctionCode += name;
    createFunctionCode += "(){\n";
    
    auto frameCounter = 0;
    auto maxPixelCounter = 0;
    for(auto frame : _recordedAnimation){
        if(!frame["pixels"].empty()){
            frameCounter++;
            createFunctionCode += "\t"+name+".addNewFrame(";
            createFunctionCode += ofToString(frame["time"].get<int>());
            createFunctionCode += ");\n";
            auto pixelCounter = 0;
            for(auto pixel : frame["pixels"]){
                pixelCounter++;
                createFunctionCode += "\t"+name+".addPixelToLastFrame(";
                createFunctionCode += ofToString(pixel["index"].get<int>());
                createFunctionCode += ",";
                createFunctionCode += ofToString(pixel["r"].get<int>());
                createFunctionCode += ",";
                createFunctionCode += ofToString(pixel["g"].get<int>());
                createFunctionCode += ",";
                createFunctionCode += ofToString(pixel["b"].get<int>());
                createFunctionCode += ");\n";
            }
            maxPixelCounter = max(maxPixelCounter, pixelCounter);
        }
    }
    createFunctionCode += "}";
    
    std::string code = "#pragma once\n";
    code += "#include \"./Animation.h\"\n";
    code += "static Animation<"+ofToString(frameCounter)+", "+ofToString(maxPixelCounter)+"> ";
    code += name + ";\n\n";
    
    output << code;
    output << createFunctionCode;

    output.close();

}

void ofApp::exportVideo(std::string path){
//    _exportVideoPlayer.loadSync(path);
}

void ofApp::onSerialBuffer(const ofx::IO::SerialBufferEventArgs &args)
{
    ofLogNotice() << "Serial message " << args.buffer().toString();
}

void ofApp::onSerialError(const ofx::IO::SerialBufferErrorEventArgs &args)
{
    ofLogError() << "Serial error " << args.buffer().toString();
}

void ofApp::onModeChange(int & value){
    switch(value){
        case INPUTMODE::INPUTMODE_VIDEOGRABBER: {
            // TODO: add device dropdown
            // _videoGrabber.setDeviceID(0);
            _videoGrabber.initGrabber(640, 480);
            break;
        }
        case INPUTMODE::INPUTMODE_NDIGRABBER: {
            _videoGrabber.close();
            stopPlayer();
            if(_selectedNdiDevice >= 0 && _selectedNdiDevice < _ndiGrabberDevices.size()){
                _ndiGrabber.setDevice(_ndiGrabberDevices[_selectedNdiDevice]);
            }
        }
        default: {
            _videoGrabber.close();
            break;
        }
    }
}

void ofApp::onMuteChange(bool & value){
    _videoPlayer.setVolume(value ? 0 : 1);
}

void ofApp::onLoopChange(bool & value){
    _videoPlayer.setLoopState(value ? ofLoopType::OF_LOOP_NORMAL : ofLoopType::OF_LOOP_NONE);
}

void ofApp::onRecordingChange(bool & value){
    if(value){
        _recordingStartedTimestamp = ofGetElapsedTimeMillis();
        _recordedAnimation.clear();
        _recordedAnimation = ofJson::array();
    }else{
        ofLogNotice() << _recordedAnimation.dump(4);
    }
}

void ofApp::onNDIDeviceChange(int & value){
    if(value >= 0 && value < _ndiGrabberDevices.size()){
        _ndiGrabber.setDevice(_ndiGrabberDevices[value]);
    }
}

void ofApp::onVideoGrabberDeviceChange(int & value){
    ofLogNotice() << "video grabber device changed " << value;
    if(value >= 0 && value < _videoGrabberDevices.size()){
        
        _videoGrabber.close();
        _videoGrabber.setDeviceID(value);
    }
}

void ofApp::onSerialPortChange(std::string & value){
    ofLogNotice() << "serial port changed " << value;
    if(value.empty()){
    }else{
        auto success = _serialDevice.setup(value, 115200);
        if(success){
            ofLogNotice() << "successfully opened port " << value;
        }else{
            ofLogNotice() << "could not open port " << value;
        }
    }
}
