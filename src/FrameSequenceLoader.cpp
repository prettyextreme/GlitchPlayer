
#include "FrameSequenceLoader.h"
#include "GlitchPlayer.h"

FrameSequenceLoader::FrameSequenceLoader(){
    
    proportionDone = 1;
    
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
    
    
    totalReadFrameCt = vid.getTotalNumFrames();
    
    
    firstReadFrameNum = 0;
    finalReadFrameNum = totalReadFrameCt;
    
    //MAKE SURE WE USE AN EVEN NUMBER OF FRAMES!
    if((finalReadFrameNum - firstReadFrameNum)%2 == 0)
        finalReadFrameNum--;
    
    frameNumToSave = 0;
    
    int fadeFrames = min(finalReadFrameNum/2, 120);
    
    while(true){
        vid.update();
        vid.nextFrame();
        if(true){
            vid.update();
            
            int currentReadFrame = vid.getCurrentFrame();
            if(currentReadFrame >= firstReadFrameNum){
                
                if(currentReadFrame == (finalReadFrameNum - fadeFrames)){
                    frameNumToSave = 0;//START OVER! We will resave these!
                }
                
                char filename[100];
                sprintf(filename,"%s/%06d.jpg", dstDir.c_str(), frameNumToSave++);
                
                if( currentReadFrame < (finalReadFrameNum - fadeFrames) ){
                    
                    //new way, no texture:
                    turboJPEG.save(vid.getPixels(), filename, vid.width, vid.height, 100);
                } else if(currentReadFrame < finalReadFrameNum){

                    //Create Crossfade
                    //unsigned char tempImgBuffer[2048*1080*3];
                    //turboJPEG.load(filename, tempImgBuffer, 2048*1080*3);
                    
                    ofImage img;
                    img.setUseTexture(false);
                    img.loadImage(filename);
                    unsigned char* tempImgBuffer = img.getPixels();
                    
                    
                    unsigned char* moviepix = vid.getPixels();
                    float imgProportion = ofMap(currentReadFrame,finalReadFrameNum - fadeFrames-1,finalReadFrameNum,0,1);
                    float vidProportion = 1.0f-imgProportion;
                    int bytesToBlend = vid.width*vid.height*3;
                    for(int i=0;i<bytesToBlend;i++){
                        tempImgBuffer[i] = imgProportion*tempImgBuffer[i]+vidProportion*moviepix[i];
                    }
                    turboJPEG.save(tempImgBuffer, filename, vid.width, vid.height, 100);
                    
                } else {
                    proportionDone = 1.0f;
                    break;
                }
                
                proportionDone = ofMap(currentReadFrame,0,totalReadFrameCt,0,1);
            }
            
        }
    }
    
    thePlayer->addVideoToSystem(dstDir, finalReadFrameNum - fadeFrames);
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
    
    
    vid.setUseTexture(false);
    
    
    if(vid.getWidth()>0)
        vid.close();
    
    vid.loadMovie(srcFile);
    vid.setPaused(true);
    
    startThread();
}