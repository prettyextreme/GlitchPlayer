
#include "FrameSequenceLoader.h"
#include "GlitchPlayer.h"

FrameSequenceLoader::FrameSequenceLoader(){
    
    proportionDone = 1;
    vid.setUseTexture(false);
    
}

void FrameSequenceLoader::threadedFunction(){
    
    
    if(dstDir.length()==0){
        printf("Error! Bad Destination\n");
        return;
    }
    
    ofDirectory checkDir;
    checkDir.open(dstDir);
    if(!checkDir.exists())
        checkDir.create();
    
    
    AVData avd = vid.getAVData();
    if(avd.m_VideoData.m_iWidth>0){
        vid.stop();
        //vid.close();
    }
    
    vid.loadMovie(srcFile);
    
    totalReadFrameCt = vid.getTotalNumFrames();
    
    //Fuck it. Just skip the first 30 frames to avoid duplicates
    firstReadFrameNum = 30;
    finalReadFrameNum = totalReadFrameCt-30;
    
    //MAKE SURE WE USE AN EVEN NUMBER OF FRAMES!
    if((finalReadFrameNum - firstReadFrameNum)%2 == 0)
        finalReadFrameNum--;
    
    frameNumToSave = 0;
    
    int fadeFrames = min((finalReadFrameNum-60)/2, 120);
    
    int width = 1280;
    int height = 720;
    avd = vid.getAVData();
    width = avd.m_VideoData.m_iWidth;
    height = avd.m_VideoData.m_iHeight;
    
    vid.play();
    
    int lastReadFrameNum = -2;
    
    while(true){
        //vid.update();
        if(true){
            vid.update();
            
            int currentReadFrame = vid.getCurrentFrame();
            
            printf("Read Frame %i\n",currentReadFrame);
            
            if(currentReadFrame > 0 && lastReadFrameNum == currentReadFrame)
                currentReadFrame = finalReadFrameNum;
            
            lastReadFrameNum = currentReadFrame;
            
            if(currentReadFrame >= firstReadFrameNum){
                
                if(currentReadFrame == (finalReadFrameNum - fadeFrames)){
                    frameNumToSave = 0;//START OVER! We will resave these!
                }
                
                char filename[100];
                sprintf(filename,"%s/%06d.jpg", dstDir.c_str(), frameNumToSave++);
                printf("%i %s\n",frameNumToSave,filename);
                
                if( currentReadFrame < (finalReadFrameNum - fadeFrames) ){
                    
                    //new way, no texture:
                    turboJPEG.save(vid.getPixels(), filename, width, height, 96);
                } else if(currentReadFrame < finalReadFrameNum){

                    //Create Crossfade
                    //unsigned char tempImgBuffer[2048*1080*3];
                    //turboJPEG.load(filename, tempImgBuffer, 2048*1080*3);
                    
                    ofImage img;
                    img.setUseTexture(false);
                    img.loadImage(filename);
                    unsigned char* tempImgBuffer = img.getPixels();
                    
                    
                    
                    if(img.width>0){
                        unsigned char* moviepix = vid.getPixels();
                        float imgProportion = ofMap(currentReadFrame,finalReadFrameNum - fadeFrames-1,finalReadFrameNum,0,1);
                        float vidProportion = 1.0f-imgProportion;
                        int bytesToBlend = width*height*3;
                        for(int i=0;i<bytesToBlend;i++){
                            tempImgBuffer[i] = imgProportion*tempImgBuffer[i]+vidProportion*moviepix[i];
                        }
                        turboJPEG.save(tempImgBuffer, filename, width, height, 96);
                    } else {
                        //not ideal, but something went wrong!
                        turboJPEG.save(vid.getPixels(), filename, width, height, 96);
                    }
                    
                } else {
                    proportionDone = 1.0f;
                    break;
                }
                
                proportionDone = ofMap(currentReadFrame,0,totalReadFrameCt,0,1);
            }
            
        }
    }
    
    thePlayer->addVideoToSystem(dstDir, finalReadFrameNum - fadeFrames - firstReadFrameNum);
    ofDirectory dir;
    dir.open(srcFile);
    dir.moveTo(copyToDir+"/"+ofFilePath::getFileName(srcFile),false,true);
    dir.close();
}

void FrameSequenceLoader::load(string sourceFile, string destPath, string copyToFolderWhenFinished, GlitchPlayer* playerToCallback){
    
    srcFile = sourceFile;
    dstDir = destPath;
    copyToDir = copyToFolderWhenFinished;
    thePlayer = playerToCallback;
    
    proportionDone = 0.0;
    
    
    //vid.setUseTexture(false);
    
    
    //vid.setPaused(true);
    
    startThread();
}