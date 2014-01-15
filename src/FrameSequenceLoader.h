#pragma once

#include "ofMain.h"
#include "ofxTurboJpeg.h"
#include "ofxFFVideoPlayer.h"

class GlitchPlayer;

class FrameSequenceLoader : protected ofThread {
public:
    FrameSequenceLoader();
    
    void load(string sourceFile, string destPath, string copyToFolderWhenFinished, GlitchPlayer* playerToCallback);
    
    float loadingStatus(){return proportionDone;}
    
    void threadedFunction();
    
    bool getIsRunning(){return isThreadRunning();}
    
private:
    
    
    ofxFFVideoPlayer vid;
    
    string srcFile;
    string dstDir;
    string copyToDir;
    volatile float proportionDone;
    
    ofxTurboJpeg turboJPEG;
    
    
    int totalReadFrameCt;
    int firstReadFrameNum;
    int finalReadFrameNum;
    int frameNumToSave;
    
    GlitchPlayer* thePlayer;
};
