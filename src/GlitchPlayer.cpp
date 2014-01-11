#include "GlitchPlayer.h"

void initScene() {

}

//--------------------------------------------------------------
void GlitchPlayer::setup(){
    
	monomeControl.setup(0);
    
    //ofSetFrameRate(60);
    

	ofBackground(255,255,255);	
	ofEnableAlphaBlending();

	ofSetWindowPosition(200,200);

	thisImage = 0;

	//oscReceiver.setup(9998);
	//oscSender.setup("127.0.0.1",9000);

	speedIndicator = 3;
	videoIndicator = 0;

	dlist = new ofDirectory();
	dlist->allowExt("");
	videosPresent = 0;
    
    vidParentDir = "/Users/Josh/Media/GlitchPlayer";
    dropDir = "/Users/Josh/Desktop/Drop";
    ingestedDir = "/Users/Josh/Desktop/Drop/Ingested";
    
    int vidsToAdd = dlist->listDir(vidParentDir);
	video = new VIDEO[MAX_VIDEOS];

	printf("%i vids\n",vidsToAdd);
	dlist->allowExt("jpg");
    
	for(int i=0;i<vidsToAdd;i++)
	{
        addVideoToSystem(dlist->getPath(i),ofCountFiles(dlist->getPath(i),".jpg"));
        
//		video[i].folder = dlist->getPath(i);
//		video[i].totalFramesX2 = ofCountFiles(video[i].folder,".jpg")*2;
//		printf("%s %i frames\n",dlist->getPath(i).c_str(),video[i].totalFramesX2/2);
//		video[i].frameNum = video[i].totalFramesX2/2; //start in the middle
	}
    
    
    //Just do this on the first update
    //monitorDirectory();

	flagRepress = false;


	monomeControl.ClearLEDs();
	
	//set first 4 columns red
	for(int i=0;i<4;i++)
	{
		monomeControl.SetCol1(i,255);
	}

	framesPer4Beats = 80;
	framesSinceTrigger = 0;
	displayMode = 0;
	muteVid = false;
	skipForward = true;
	skipAmount = 1;
	OSCFade = false;
	OSCFadeLevel = 0;
	OSCFramesSinceReceive = 1000;
	
    ofDisableDataPath();
    
    /*
	for(int i=0;i<videosPresent;i++)
	{
		ofImage tempImage;
		//tempImage.loadImage(video[i].folder+"/000000.jpg");
        turboJpeg.load(video[i].folder+"/000000.jpg", &tempImage);
		video[i].width = tempImage.width;
		video[i].height = tempImage.height;
		video[i].effect = 2;     //0
		video[i].effectFrame = 0;
		video[i].effectEngaged = false;  //false
	}*/
    
    videoPage = 0;

	monomeMode = PLAYBACK;
	
	effectColor.r = 128;
	effectColor.g = 0;
	effectColor.b = 0;
	effectColor.midpoint = 128;
	effectColor.ratio = 0.9;

	//erosionShader.SetColor(effectColor);

	//erosionShader.SetErosionRadius(5);

	JitteryEngage = 0;
	FramesSinceJitteryHit = 1000;
	FramesSinceBumpPulse = 1000;

	movekeyCurrent = 0;
	accumulatorCurrent = 0;

	leftFBO.allocate(ofGetWidth(),ofGetHeight(),GL_RGBA);
	rightFBO.allocate(ofGetWidth(),ofGetHeight(),GL_RGBA);

	movekeyResetFlag = true;
	//_flock = new CFlock(1000,500,500,500);


  //tjDecompressToYUV(jpeg, buf, size, BMPbuf, 0);
  
	//tjDecompress(
	//turboJpeg.load("C:\\Users\\Joshua Silverman\\Desktop\\Dropbox\\Photos\\Sample Album\\Frog.jpg",&tempImg );

	totallyRandomFrame = false;
    
    
    post.init(1280,720);
    post.setFlip(false);
    //post.createPass<FxaaPass>()->setEnabled(false);
    
    offsetPass = post.createPass<OffsetPass>();
    offsetPass->setEnabled(false);
    
    colorizePass = post.createPass<ColorizePass>();
    colorizePass->setEnabled(false);
    
    
    
    ofFbo::Settings s;
    s.width = ofNextPow2(1280);
    s.height = ofNextPow2(720);
    s.textureTarget = GL_TEXTURE_2D;
    offsetFbo.allocate(s);
    
    s.width = ofNextPow2(2048);
    s.height = ofNextPow2(1);
    offsetFboWide.allocate(s);
    
    s.width = ofNextPow2(1);
    s.height = ofNextPow2(1024);
    offsetFboTall.allocate(s);
    
    keyboardMode = KMODE_VID;
    
    initEdgeNoise();

    ToggleMonomeMode();//redraw
}

void GlitchPlayer::addVideoToSystem(string folder, int totalFrames){
    
    if(videosPresent>=MAX_VIDEOS){
        printf("Error: Out of space!\n");
        return;
    }
    
    video[videosPresent].folder = folder;
    video[videosPresent].totalFramesX2 = totalFrames*2;
    printf("%s %i frames\n",folder.c_str(),video[videosPresent].totalFramesX2/2);
    video[videosPresent].frameNum = video[videosPresent].totalFramesX2/2; //start in the middle
    
    ofImage tempImage;
    tempImage.setUseTexture(false);
    tempImage.loadImage(video[videosPresent].folder+"/000000.jpg");
    //turboJpeg.load(video[videosPresent].folder+"/000000.jpg", &tempImage);
    video[videosPresent].width = tempImage.width;
    video[videosPresent].height = tempImage.height;
    video[videosPresent].effect = 2;     //0
    video[videosPresent].effectFrame = 0;
    video[videosPresent].effectEngaged = false;  //false
    
    
    videosPresent++;
    
    ToggleMonomeMode();
}

void GlitchPlayer::monitorDirectory(){
    //check the monitored directory to find anything new since last time:
    
    //Don't even look if it's currently loading something
    if(fsLoader.loadingStatus() != 1.0f || fsLoader.getIsRunning() || ofGetFrameNum()%60!=0)
        return;
    
    ofDirectory dropDirectory;
    
    dropDirectory.listDir("/Users/Josh/Desktop/Drop");
    
    for(int i=0;i<dropDirectory.numFiles();i++){
        ofFile aFile = dropDirectory.getFile(i);
        
        bool isMov = false;
        if(aFile.path().find(".MOV")!=string::npos)
            isMov = true;
        if(aFile.path().find(".mov")!=string::npos)
            isMov = true;
        if(aFile.path().find(".Mov")!=string::npos)
            isMov = true;
        
        if(isMov){
            //START LOADING HERE
            
            char dstPath[256];
            sprintf(dstPath, "%s/%02i",vidParentDir.c_str(), videosPresent);
            fsLoader.load(aFile.path(), dstPath, ingestedDir, this);
            
            printf("Loading: %s\n",aFile.path().c_str());
            
            break;
            
            //When it's done, we will move it to Drop/Ingested
        }
        
    }
    
}

//--------------------------------------------------------------
void GlitchPlayer::update(){
    
    if(ofGetFrameNum()==1){
        ofSetWindowPosition(0, 0);
        ofHideCursor();
    }
    
    monitorDirectory();
    
	//_flock->Update(mouseX,mouseY);

//
//	while(	oscReceiver.hasWaitingMessages())
//	{
//		ofxOscMessage m;
//		oscReceiver.getNextMessage(&m);
//			
//		if(m.getAddress().compare("/tuio/2Dcur")==0)
//		{
//			OSCFramesSinceReceive = 0;
//			static bool lastWasSource = false;
//			
//			if(m.getArgAsString(0).compare("alive")==0 && lastWasSource)
//			{
//				//indicates xy release
//				OSCFade = false;
//			}
//
//
//			if(m.getArgAsString(0).compare("source")==0)
//				lastWasSource = true;
//			else
//				lastWasSource = false;
//
//			if(m.getArgAsString(0).compare("set")==0)
//			{				
//				if(m.getNumArgs()>3)
//				{
//					OSCFade = true;
//					OSCFadeLevel = m.getArgAsFloat(3);
//				}
//				/*
//				if(m.getArgAsInt32(1)==1)
//					printf("%s %i %f %f\n",m.getArgAsString(0).c_str(),m.getArgAsInt32(1),m.getArgAsFloat(2),m.getArgAsFloat(3));
//				else
//					printf("");*/
//			}
//		}
//
//	}

	int row=0,col=0;
	bool buttonDown = false;
	while(monomeControl.GetButtonPress(col,row,buttonDown))
	{
		
		if(col==2 && row==2)
		{
			totallyRandomFrame = buttonDown;
		}
		if(col==0 && row==0)
		{
			muteVid = buttonDown;
		}
		if(col==2 && row==0)
		{
			//toggle between monomeModes
			switch(buttonDown?1:0)
			{
			case 0: 
				monomeMode = PLAYBACK;
				break;
			case 1:
				monomeMode = EFFECT;
				break;
			}
			/*
			ofxOscMessage m2;
			m2.clear();
			m2.setAddress("/128/led");
			m2.addIntArg(col);
			m2.addIntArg(row);
			m2.addIntArg(monomeMode == PLAYBACK?1:0);
			oscSender.sendMessage(m);*/
			monomeControl.SetLED(col,row,monomeMode == PLAYBACK);
			ToggleMonomeMode();
		}
		if(col==2 && row==1)
		{
			//toggle between monomeModes
			switch(buttonDown?1:0)
			{
			case 0: 
				monomeMode = PLAYBACK;
				break;
			case 1:
				monomeMode = COLOR;
				break;
			}
			/*
			ofxOscMessage m2;
			m2.clear();
			m2.setAddress("/128/led");
			m2.addIntArg(col);
			m2.addIntArg(row);
			m2.addIntArg(monomeMode == PLAYBACK?1:0);
			oscSender.sendMessage(m);*/
			monomeControl.SetLED(col,row,monomeMode == PLAYBACK);
			ToggleMonomeMode();
		}
		if(col==0 && row==1 && buttonDown)
		{
			flashWhite = true; 
		}
		if(col==1 && row==5 && buttonDown)
		{
			JitteryEngage = 0;
			FramesSinceBumpPulse = 0;
		}
		if(col==1 && row==6)
		{
			JitteryEngage = buttonDown?1:0;
			JitteryFrame = 2*ofRandom(0,1000);
		}
		if(col==1 && row==7 && buttonDown)
		{
			//JitteryEngage = m.getArgAsInt32(2)==1?2:0;
			JitteryFrame = 2*ofRandom(0,1000);
			FramesSinceJitteryHit = 0;
		}
		if(col==3 && buttonDown)
		{
            int lastVideoPage = videoPage;
            videoPage = row;
			ToggleMonomeMode();
            
            if(videoIndicator >= videoPage*12 && videoIndicator < (videoPage+1)*12 ){
                monomeControl.SetLED(videoIndicator%12+4,speedIndicator,true);
            }
		}
		if(buttonDown)
		{
			//first 4 columns reserved for other shit!
			//bottom 2 row for other control
			if(col>=4)
			{						
				switch(monomeMode)
				{
				case PLAYBACK:
                    {
                        int vidIndex = (col-4)+videoPage*12;
                        if(row<=5)
                        {
                            if(vidIndex>=videosPresent)
                                return;

                            framesSinceTrigger = 0;
                            if(speedIndicator == row && videoIndicator == vidIndex)
                                flagRepress = true;
                            else
                            {

                                int lastSpeedIndicator = speedIndicator ;
                                int lastVideoIndicator = videoIndicator ;
                                int lastVideoPage      = videoPage;

                                if(vidIndex >= videoPage*12 && vidIndex < (videoPage+1)*12 )
                                    monomeControl.SetLED(videoIndicator%12+4,speedIndicator,false);
                                
                                speedIndicator = row;
                                videoIndicator = vidIndex;
                                    
                                //m.setAddress("/128/led");
                                //oscSender.sendMessage(m);
                                monomeControl.SetLED(col,row,true);
                                /*
                                ofxOscMessage m2;
                                m2.clear();
                                m2.setAddress("/128/led");
                                m2.addIntArg(lastVideoIndicator+4);
                                m2.addIntArg(lastSpeedIndicator);
                                m2.addIntArg(0);
                                oscSender.sendMessage(m2);*/
                                
                            }
                        }
                        else if(row == 6)
                        {
                            if(vidIndex>=videosPresent)
                                return;

                            video[vidIndex].effectEngaged = !video[vidIndex].effectEngaged;

                            /*
                            ofxOscMessage m2;
                            m2.clear();
                            m2.setAddress("/128/led");
                            m2.addIntArg(col);
                            m2.addIntArg(6);
                            m2.addIntArg(video[col-4].effectEngaged?1:0);
                            oscSender.sendMessage(m2);*/
                                
                            monomeControl.SetLED(col,6,video[vidIndex].effectEngaged);
                        }
                    }
					break;
				case EFFECT:
					if(buttonDown)
					{
                        int vidIndex = (col-4)+videoPage*12;
						if(vidIndex<videosPresent)
						{
							video[vidIndex].effect = row;
							monomeControl.SetCol1(col,1<<video[vidIndex].effect);
							/*
							ofxOscMessage m2;
							m2.clear();
							m2.setAddress("/128/led_col");
							m2.addIntArg(col);
							m2.addIntArg(1<<video[vid].effect);	
							oscSender.sendMessage(m2);		*/
						}
					}
					break;
				case COLOR:
					if(buttonDown)
					{
						switch(row)
						{
						case 0:
							effectColor.r = ofMap(col,4,15,0,255);
							break;
						case 1:
							effectColor.g = ofMap(col,4,15,0,255);
							break;
						case 2:
							effectColor.b = ofMap(col,4,15,0,255);
							break;
						case 3:
							effectColor.midpoint = ofMap(col,4,15,0,255);
							break;
						case 4:
							effectColor.ratio = ofMap(col,4,15,0,1);
							break;
						}
						//erosionShader.SetColor(effectColor);
						sendMonomeColors();
					}
					break;
				}
			}
			else
			{
				//measure duration of 4 beats
				if(col==0 && row==7)
				{
					if(ofGetFrameNum() > (frameToStartCounting+200)) //start measuring fresh
					{
						frameToStartCounting = ofGetFrameNum();
					}else{
						framesPer4Beats = (ofGetFrameNum()-frameToStartCounting);
						printf("Frames Per 4 Beats: %i\n",framesPer4Beats);
					}
				}
				if(col==0 && row==2)
				{
					skipForward = !skipForward;
					/*
					ofxOscMessage m2;
					m2.clear();
					m2.setAddress("/128/led");
					m2.addIntArg(0);
					m2.addIntArg(2);
					m2.addIntArg(skipForward?1:0);
					oscSender.sendMessage(m2);*/
						
					monomeControl.SetLED(0,2,skipForward);
				}
				if(col==0 && row>=3 && row<=6)
				{/*
					ofxOscMessage m2;
					m2.clear();
					m2.setAddress("/128/led");
					m2.addIntArg(0);
					m2.addIntArg(skipAmount+3);
					m2.addIntArg(1);
					oscSender.sendMessage(m2);*/
					monomeControl.SetLED(0,skipAmount+3,1);
					
					skipAmount = row-3;
					/*
					m2.clear();
					m2.setAddress("/128/led");
					m2.addIntArg(0);
					m2.addIntArg(skipAmount+3);
					m2.addIntArg(0);
					oscSender.sendMessage(m2);*/
					monomeControl.SetLED(0,skipAmount+3,0);
				}
				if(col==1)
				{
					if(row<5)
					{
						monomeControl.SetCol1(1,255);
						monomeControl.SetLED(1,row,false);
						/*
						ofxOscMessage m2;
						m2.clear();
						m2.setAddress("/128/led_col");
						m2.addIntArg(1);
						m2.addIntArg(255);
						oscSender.sendMessage(m2);
						m2.clear();
						m2.setAddress("/128/led");
						m2.addIntArg(1);
						m2.addIntArg(row);
						m2.addIntArg(0);
						oscSender.sendMessage(m2);
						*/
						displayMode = row;

						if(row==1)
							framesSinceTrigger=0;//make sure we get the fade going on this frame
					}
				}
			}
			
		}
		/*
		ofxOscMessage m2;
		m2.clear();
		m2.setAddress("/128/led");
		m2.addIntArg(2);
		m2.addIntArg(255);
		oscSender.sendMessage(m2);*/
	}

	//_flock.update();
	//_flock.update();
	//_flock.update();
}

void GlitchPlayer::exit()
{

}

void GlitchPlayer::sendMonomeColors()
{
	for(int i=4;i<16;i++)
	{
		monomeControl.SetCol1(i,0);
		/*
		ofxOscMessage m2;
		m2.clear();
		m2.setAddress("/128/led_col");
		m2.addIntArg(i);
		m2.addIntArg(0);	
		oscSender.sendMessage(m2);	*/	
	}

	for(int i=0;i<5;i++)
	{/*
		ofxOscMessage m2;
		m2.clear();
		m2.setAddress("/128/led");
		switch(i)
		{
		case 0:
			m2.addIntArg(ofMap(effectColor.r,0,255,4,15));
			break;
		case 1:
			m2.addIntArg(ofMap(effectColor.g,0,255,4,15));
			break;
		case 2:
			m2.addIntArg(ofMap(effectColor.b,0,255,4,15));
			break;
		case 3:
			m2.addIntArg(ofMap(effectColor.midpoint,0,255,4,15));
			break;
		case 4:
			m2.addIntArg(ofMap(effectColor.ratio,0,1,4,15));
			break;
		default:
			continue;
			break;
		}
		m2.addIntArg(i);	
		m2.addIntArg(1);	
		oscSender.sendMessage(m2);	
*/
		int m2 = 0;

		switch(i)
		{
		case 0:
			m2 = ofMap(effectColor.r,0,255,4,15);
			break;
		case 1:
			m2 = ofMap(effectColor.g,0,255,4,15);
			break;
		case 2:
			m2 = ofMap(effectColor.b,0,255,4,15);
			break;
		case 3:
			m2 = ofMap(effectColor.midpoint,0,255,4,15);
			break;
		case 4:
			m2 = ofMap(effectColor.ratio,0,1,4,15);
			break;
		default:
			continue;
			break;
		}
		monomeControl.SetLED(m2,i,true);
	}
}

void GlitchPlayer::ToggleMonomeMode()
{
	switch(monomeMode)
	{
	case EFFECT:

        {
            
            int startVid = videoPage*12;
            int toVid = startVid+12;
            for(int i = startVid, j=4;i<toVid;i++, j++)
            {
                monomeControl.SetCol1(j,1<<video[i].effect);
                /*
                ofxOscMessage m2;
                m2.clear();
                m2.setAddress("/128/led_col");
                m2.addIntArg(i+4);
                m2.addIntArg(1<<video[i].effect);	
                oscSender.sendMessage(m2);	*/	
            }
        }

		break;
	case COLOR:
		sendMonomeColors();
		break;
	case PLAYBACK:
	default:
        {
            int startVid = videoPage*12;
            int toVid = startVid+12;
            for(int i = startVid, j=4;i<toVid;i++,j++)
            {
                if(i<videosPresent)
                    monomeControl.SetCol1(j,video[i].effectEngaged<<6);
                else
                    monomeControl.SetCol1(j,3);
            }
            
            
            if(videoIndicator >= videoPage*12 && videoIndicator < (videoPage+1)*12 ){
                monomeControl.SetLED(videoIndicator%12+4,speedIndicator,true);
            }
            
            //monomeControl.SetLED(videoIndicator+4,speedIndicator,1);
            
            
            if(videoIndicator >= videoPage*12 && videoIndicator < (videoPage+1)*12 ){
                monomeControl.SetLED(videoIndicator%12+4,speedIndicator,true);
            }
            
            monomeControl.SetCol1(3, 255);
            monomeControl.SetLED(3,videoPage,false);
        }

		break;
	}

}

//--------------------------------------------------------------
void GlitchPlayer::draw(){
	/*
	static ofImage img;
	char filename2[1000];
	sprintf(filename2,"c:\\media\\03\\%.6i.jpg",ofGetFrameNum()%300);
	turboJpeg.load(filename2, &img);		
	img.draw(0,0);
	ofDrawBitmapString(ofToString(ofGetFrameRate(),3),20,20);

	return;
	*/

    
    
    ofViewport(ofRectangle(0,-ofGetHeight(),ofGetWidth()*2,ofGetHeight()*2));








	ofBackground(0,40,0);

	bool writeToFile = false;
	if(writeToFile)
	{
		if(fboForWritingOutput.getHeight() != ofGetHeight() || fboForWritingOutput.getWidth() != ofGetWidth())
		{
			fboForWritingOutput.allocate(ofGetWidth(), ofGetHeight(), GL_RGB);
		}
		fboForWritingOutput.begin();
		//flip before doing any rendering! otherwise it saves upside down!
		ofTranslate(0,ofGetHeight());
		ofScale(1,-1); 
	}
/*
	leftFBO.begin();
	leftFBO.clear(0,0,0,0);
	ofSetColor(100,100,0);
	ofRect(150,150,200,200);
	ofSetColor(255,255,255);
	ofDrawBitmapString("L",160,160);
	leftFBO.end();

	rightFBO.begin();
	rightFBO.clear(0,0,0,0);
	ofSetColor(100,0,100,200);
	ofRect(200,50,200,200);	
	ofSetColor(255,255,255);
	ofDrawBitmapString("R R R",210,60);
	rightFBO.end();
			*/
		//return;

		//movekeyFBO[movekeyCurrent].begin();
/*
	return;
				
		int shaderProgID = movekeyShader.getShaderProgramID();

		movekeyShader.Begin();

		ofTextureData td0 = rightFBO.getTextureData();
		glActiveTexture(GL_TEXTURE0_ARB);
		glEnable(td0.textureTarget);
		glBindTexture(td0.textureTarget,td0.textureID );
		glUniform1i(glGetUniformLocation(shaderProgID, "tex0"), 0);

		ofTextureData td1 = leftFBO.getTextureData();
		glActiveTexture(GL_TEXTURE1_ARB);
		glEnable(td1.textureTarget);
		glBindTexture(td1.textureTarget,td1.textureID );
		glUniform1i(glGetUniformLocation(shaderProgID, "tex1"), 1);
		
		float w = ofGetWidth();
		float h = ofGetHeight();

		glBegin(GL_QUADS);
			glMultiTexCoord2fARB(GL_TEXTURE0, 0, h);
			glMultiTexCoord2fARB(GL_TEXTURE1, 0, h);  
			glVertex3f(0.0, 0.0, 0.0);

			glMultiTexCoord2fARB(GL_TEXTURE0, w, h);
			glMultiTexCoord2fARB(GL_TEXTURE1, w, h);
			glVertex3f(w, 0, 0.0);

			glMultiTexCoord2fARB(GL_TEXTURE0, w, 0);
			glMultiTexCoord2fARB(GL_TEXTURE1, w, 0); 
			glVertex3f(w, h, 0.0);

			glMultiTexCoord2fARB(GL_TEXTURE0, 0, 0);
			glMultiTexCoord2fARB(GL_TEXTURE1, 0, 0);
			glVertex3f(0,h, 0.0);
		glEnd();

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glDisable(td0.textureTarget);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(td1.textureTarget);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		movekeyShader.End();
 

		return;

*/


























	ofSetColor(255,255,255,255);

	if(currentVidFBO.getHeight() != ofGetHeight() || currentVidFBO.getWidth() != ofGetWidth())
	{
		currentVidFBO.allocate(ofGetWidth(), ofGetHeight(), GL_RGB);
	}
	for(int i=0;i<4;i++)
	if(accumulatorFBO[i].getHeight() != ofGetHeight() || accumulatorFBO[i].getWidth() != ofGetWidth())
	{
		accumulatorFBO[i].allocate(ofGetWidth(), ofGetHeight(), GL_RGB);
//		accumulatorFBO[i].clear(255,255,255,255);
	}

	for(int i=0;i<4;i++)
	{
		if(movekeyFBO[i].getHeight() != ofGetHeight() || movekeyFBO[i].getWidth() != ofGetWidth())
		{
			movekeyFBO[i].allocate(ofGetWidth(), ofGetHeight(), GL_RGB);
//			movekeyFBO[i].clear(255,255,255,255);
		}
	}


	//static int framenumX2 = 0;
	static int lastSpeedIndicator = 0;
	static int lastVideoIndicator = 0;
	/*if(ofGetFrameNum()%2==0)
	{
		//ofRect(0,0,ofGetWidth(),ofGetHeight());
		return;
	}*/

	/*
	0 rev 2x
	1 rev 1x
	2 rev .5x
	3 freeze
	4 fow .5x
	5 for 1x
	6 fow 2x
	*/

	
	ofRectangle imgRegion(0,0,video[videoIndicator].width,video[videoIndicator].height);
	ofRectangle winRegion(0,0,ofGetWidth(),ofGetHeight()); //for drawing

	ofRectangle dispRegion = ofMaintainAndCenter(imgRegion,winRegion);
	


	int framenumX2 = video[videoIndicator].frameNum;

	//int totalFrames = 142*4;

	//framenumX2 += video[videoIndicator].totalFramesX2;//lets keep this wel away from 0. We'll mod it later.
	

	//else
	{
        
        
        //Old speeds:
//		int md = 0;
//		switch(speedIndicator)
//		{
//		case 0://0 rev 2x		
//			//framenumX2-=4;
//			framenumX2 += (video[videoIndicator].totalFramesX2-4);
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			md = framenumX2%4;
//			if(md!=0)
//				framenumX2-=md;
//			break;
//		case 1://rev 1x
//			//framenumX2-=2;
//			framenumX2 += (video[videoIndicator].totalFramesX2-2);
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			md = framenumX2%4;
//			if(md==1 || md==3)
//				framenumX2-=1;
//			break;
//		case 2://rev .5
//			//framenumX2--;
//			framenumX2 += (video[videoIndicator].totalFramesX2-1);
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			break;
///*		case 3://freeze		
//			if(md==1 || md==3)
//				framenumX2-=1;	
//			break;*/
//		case 3://.5
//			framenumX2++;
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			break;
//		case 4://1x
//			framenumX2+=2;
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			md = framenumX2%4;
//			if(md==1 || md==3)
//				framenumX2+=1;
//			break;
//		case 5://2x
//			framenumX2+=4;
//			framenumX2 %= video[videoIndicator].totalFramesX2;
//			md = framenumX2%4;
//			if(md!=0)
//				framenumX2+=(1-md);
//			break;
//		}
        
        
        //New speeds for 120fps content
        int md = 0;
		switch(speedIndicator)
		{
            case 0://0 rev 4x
                //framenumX2-=4;
                framenumX2 += (video[videoIndicator].totalFramesX2-8);
                framenumX2 %= video[videoIndicator].totalFramesX2;
                md = framenumX2%8;
                if(md!=0)
                    framenumX2-=md;
                break;
            case 1://rev 2x
                //framenumX2-=2;
                framenumX2 += (video[videoIndicator].totalFramesX2-4);
                framenumX2 %= video[videoIndicator].totalFramesX2;
                md = framenumX2%8;
                if(md==1 || md==3)
                    framenumX2-=1;
                break;
            case 2://rev 1
                //framenumX2--;
                framenumX2 += (video[videoIndicator].totalFramesX2-1);
                framenumX2 %= video[videoIndicator].totalFramesX2;
                break;
                /*		case 3://freeze
                 if(md==1 || md==3)
                 framenumX2-=1;
                 break;*/
            case 3://1
                framenumX2++;
                framenumX2 %= video[videoIndicator].totalFramesX2;
                break;
            case 4://2x
                framenumX2+=4;
                framenumX2 %= video[videoIndicator].totalFramesX2;
                md = framenumX2%8;
                if(md==1 || md==3)
                    framenumX2+=1;
                break;
            case 5://4x
                framenumX2+=8;
                framenumX2 %= video[videoIndicator].totalFramesX2;
                md = framenumX2%8;
                if(md!=0)
                    framenumX2+=(1-md);
                break;
		}
	}

	if(flagRepress)
	{
		int skipframes = 0;
		switch(skipAmount)
		{
		case 0:
			skipframes = 8;
			break;
		case 1:
			skipframes = 24;
			break;
		case 2:
			skipframes = 48;
			break;
		case 3:
			skipframes = video[videoIndicator].totalFramesX2/2;//80;
			break;
		}

		if((speedIndicator>=3 && skipForward) || (speedIndicator<3 && !skipForward))//forward
		{
			framenumX2+=skipframes;
			//if we're within 30 frames of the end, then cut to frame 30
			if(framenumX2>(video[videoIndicator].totalFramesX2-60))
				framenumX2=60;
		}
		else   //if(speedIndicator<3)
		{
			framenumX2-=skipframes;
			//if we're within 30 frames of the start, cut to frame f-30
			if(framenumX2<60)
				framenumX2=video[videoIndicator].totalFramesX2-60;
		}
		framenumX2 = framenumX2*4/4;
		flagRepress = false;
	}

	//lets not loop here.
	/*if(speedIndicator>=3 && framenumX2>(video[videoIndicator].totalFramesX2-4))
		framenumX2 = 0;
	else if(speedIndicator<3 && framenumX2<4)
		framenumX2 = video[videoIndicator].totalFramesX2-4;
*/

	//make sure that if we're just popping into this vid, we don't blend with the previous frame
	if(framenumX2%2==1 && lastVideoIndicator!=videoIndicator)
		framenumX2++;

	while(framenumX2 >= video[videoIndicator].totalFramesX2)
		framenumX2 -= video[videoIndicator].totalFramesX2;
	
	while(framenumX2 < 0)
		framenumX2 += video[videoIndicator].totalFramesX2;

	
	int useVideoIndicator = videoIndicator;
	
	if(totallyRandomFrame)
	{
		useVideoIndicator = ofGetFrameNum()/2%videosPresent;
		framenumX2 = ofGetFrameNum()*12%video[useVideoIndicator].totalFramesX2;//ofRandom(0,video[videoIndicator].totalFramesX2);
		if(framenumX2%2==1)
			framenumX2--;
	}
	else
		video[useVideoIndicator].frameNum = framenumX2;
	/*
	if(lastVideoIndicator != videoIndicator)
	{
		//drop into the middle;
		framenumX2 = (totalFrames/2/4)*4;
	}*/

	lastSpeedIndicator = speedIndicator;
/*
	framenum++;
	float useFrame = (float)(framenum%200)/2.0f;
	int thisFrame = useFrame;
	int nextFrame = ((framenum+1)%200)/2;*/
	char filename[128];
	std::string recordPath = "/Users/Josh/Media/GlitchPlayer";//"c:\\media";


	
	//if we're %4==0, then just load the right frame and be done with it.
/*
	if(speedIndicator==3)
	{
		if(lastVideoIndicator != videoIndicator)
		{
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),videoIndicator, framenumX2/2);
			mainImage[thisImage].loadImage(filename);
		}
		ofSetColor(255,255,255,255);
		mainImage[thisImage].draw(0,0,ofGetWidth(),ofGetHeight());			
	}
	else */

	bool useFBO = true;
	if(useFBO)
	{
		currentVidFBO.begin();
		//glClearColor(0,0,0,0);
		//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	
	ofPushMatrix();

	bool makeJittery = JitteryEngage>0 || FramesSinceJitteryHit<100;
	if(makeJittery)
	{
		float clampMax = 1.2;
		float maxScale = ofClamp(ofMap(FramesSinceJitteryHit,0,30,clampMax,1),1,clampMax);

		if(JitteryEngage==1)
			maxScale = clampMax;

		if (JitteryFrame%2==0){
			
			JitterySettings.scale = ofMap(noise.noise(0.5*JitteryFrame),-1,1,1,maxScale);
			float widthoffset = (JitterySettings.scale-1.0f) * dispRegion.width;
			float heightoffset = (JitterySettings.scale-1.0f) * dispRegion.height;
			JitterySettings.left = ofMap(noise.noise((0.5*JitteryFrame+100)),-1,1,-widthoffset,0);
			JitterySettings.top = ofMap(noise.noise((0.5*JitteryFrame+200)),-1,1,-heightoffset,0);
/*
			JitterySettings.top = ofRandom(-randRange,0);
			JitterySettings.bottom = ofRandom(ofGetHeight(),ofGetHeight()+randRange);
			JitterySettings.left = ofRandom(-randRange,0);
			JitterySettings.right = ofRandom(ofGetWidth(),ofGetWidth()+randRange);*/
		}
		JitteryFrame++;
		/*
		winRegion.H = JitterySettings.bottom-JitterySettings.top;
		winRegion.W = JitterySettings.right-JitterySettings.left;
		winRegion.X = JitterySettings.left;
		winRegion.Y = JitterySettings.top;*/

		ofTranslate(JitterySettings.left,JitterySettings.top);
		ofScale(JitterySettings.scale,JitterySettings.scale);

		FramesSinceJitteryHit++;
		FramesSinceJitteryHit = min(FramesSinceJitteryHit,1000);
	}

	if(FramesSinceBumpPulse < 100)
	{
		float clampMax = 1.15;
		float xval = ofMap(FramesSinceBumpPulse,0,12,1,0);
		xval = max(0.0f,xval);
		float scale = ofMap(xval*xval,0,1,1,clampMax);
		float widthoffset = (scale-1.0f) * dispRegion.width;
		widthoffset/=2.0;
		widthoffset = widthoffset - xval*ofRandom(-4,4);
		float heightoffset = (scale-1.0f) * dispRegion.height;
		heightoffset/=2.0;
		heightoffset = heightoffset - xval*ofRandom(-4,4);
		ofTranslate(-widthoffset,-heightoffset);
		ofScale(scale,scale);
		
		FramesSinceBumpPulse++;
		FramesSinceBumpPulse = min(FramesSinceBumpPulse,1000);
	}
    
    //This is the old way we did it. It presumed that we had crappier content that wasn't shot at a very high frame rate.
    //We were prepared to double frames, etc.
	//printf("%i\n",(int)ofGetFrameRate());

	if(framenumX2%4==0)
	{
		
		thisImage = 1-thisImage;//toggle which image to use next
		sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2);
		turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);

		ofSetColor(255,255,255,255);
		mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	

	}
	else if(framenumX2%4==1)
	{
		//must be 0.5 speed
		if(speedIndicator>=3)//forward
		{
			//better load-up next file
			
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2+1);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);
			
			ofSetColor(255,255,255,255);
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
		
			
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			}
		}
		if(speedIndicator<3)//backwards
		{
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);

			ofSetColor(255,255,255,255);
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);		
			}
		}
	}
	else if(framenumX2%4==3)
	{
		//must be 0.5 speed
		if(speedIndicator>=3)//forward
		{
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2+1);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);

			ofSetColor(255,255,255,255);
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);		
			}
		}
		if(speedIndicator<3)//backwards
		{				
			//better load-up next file
			
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),videoIndicator, framenumX2/2);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);
			
			ofSetColor(255,255,255,255);
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
		
			
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			}
		}
	}
	else if(framenumX2%4==2)
	{
		//must be 1x or 0.5 speed
		if(speedIndicator==4)//forward 1x
		{
			//better load-up next file
			
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2);//+1);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);
			
			ofSetColor(255,255,255,255);

			
			mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
				/*
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
		
			
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			}*/
		}
		if(speedIndicator==1)//backwards 1x
		{				
			//better load-up next file
			
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);
			
			ofSetColor(255,255,255,255);
			mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
				/*
			mainImage[1-thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
		
			
			if(displayMode!=2)//don't blend if we're busy flashing our shit all over the place!
			{
				ofSetColor(255,255,255,255/2);
				mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
			}*/
		}
		if(speedIndicator==3 || speedIndicator==2)//0.5
		{
			thisImage = 1-thisImage;//toggle which image to use next
			sprintf(filename,"%s/%.2i/%.6i.jpg",recordPath.c_str(),useVideoIndicator, framenumX2/2);
			turboJpeg.load(filename,&mainImage[thisImage]);//mainImage[thisImage].loadImage(filename);

			ofSetColor(255,255,255,255);
			mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);	
		}
	}

	ofPopMatrix();

	if(dispRegion.x!=0 || dispRegion.y!=0)
	{
		ofSetColor(0,0,0);
		if(dispRegion.x!=0)//draw side bars
		{
			ofRect(0,0,dispRegion.x,dispRegion.height);
			ofRect(dispRegion.x+dispRegion.width,0,dispRegion.x,dispRegion.height);
		} 
		else//draw top/bottom bars
		{
			ofRect(0,0,dispRegion.width,dispRegion.y);
			ofRect(0,dispRegion.y+dispRegion.height,dispRegion.width,dispRegion.y+dispRegion.height);
		}
	}


	if(useFBO)
		currentVidFBO.end();

/*
//testing out motion detection:
	
	if(tinyFBO.getHeight() != 100 || tinyFBO.getWidth() != 100)
	{
		tinyFBO.allocate(100, 100, GL_LUMINANCE);
	}

	tinyFBO.begin();
	ofSetColor(255,255,255);
	currentVidFBO.draw(0,0,100,100);
	tinyFBO.end();

	if(gs[0].width != 100)
		gs[0].allocate(100,100);
	gs[0].setFromPixels((unsigned char*)tinyFBO.getPixels(),100,100);

	gs[0].threshold(200,true);
	//ci.
	gs[0].draw(0,0,ofGetWidth(),ofGetHeight());
	return;


*/






/*
	leftFBO.begin();
	leftFBO.clear(0,0,0,0);
	ofSetColor(255,0,0);
	ofRect(50,50,100,100,10);	
	leftFBO.end();
	rightFBO.begin();
	rightFBO.clear(0,0,0,0);
	ofSetColor(100,0,255);
	ofRect(200,50,100,100,10);	
	rightFBO.end();
*/
	
	//apply movekey/trails shader

	if(false)//apply tails
	{
		
		int keysInBuffer = 4;

		//copy this frame into buffer
		movekeyCurrent = (movekeyCurrent+1)%keysInBuffer;
		movekeyFBO[movekeyCurrent].begin();
			ofSetColor(255,255,255,255);
			currentVidFBO.draw(0,0);		
		movekeyFBO[movekeyCurrent].end();

		int srcID1 = movekeyCurrent;
		int srcID2 = (movekeyCurrent+keysInBuffer-1)%keysInBuffer;
		int srcID3 = (movekeyCurrent+keysInBuffer-2)%keysInBuffer;
		
		currentVidFBO.begin();
		//movekeyShader.ApplyToTextures(&accumulatorFBO[accumulatorCurrent], &movekeyFBO[srcID1], &movekeyFBO[srcID2], &movekeyFBO[srcID3]);
		currentVidFBO.end();

		
		//copy accumulator into current
		accumulatorFBO[accumulatorCurrent].begin();
		ofSetColor(255,255,255,255);
		currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
		accumulatorFBO[accumulatorCurrent].end();
		
		accumulatorFBO[accumulatorCurrent].draw(0,0,ofGetWidth(),ofGetHeight());

		//accumulatorCurrent = (accumulatorCurrent+3)%4;

		//accumulatorFBO

			//from when only 2 movekeys in buffer
			/*
		movekeyFBO[movekeyCurrent].begin();
		if(movekeyResetFlag)
		{
			ofSetColor(255,255,255,255);
			currentVidFBO.draw(0,0);
			movekeyResetFlag = false;
		} else
			movekeyShader.ApplyToTextures(&currentVidFBO, &movekeyFBO[1-movekeyCurrent], NULL, NULL, NULL);
		movekeyFBO[movekeyCurrent].end();
		ofSetColor(255,255,255,255);
		movekeyFBO[movekeyCurrent].draw(0,0,ofGetWidth(),ofGetHeight());

		movekeyCurrent = 1-movekeyCurrent;
*/




	}
	else
	{
		//Render FBO and apply any necessary effecrt
		if(!video[useVideoIndicator].effectEngaged)
		{
			ofSetColor(255,255,255,255);
			currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
		}
		else
		{
			switch(video[useVideoIndicator].effect)
			{
                default:
                case 0:
                    //erosionShader.Begin();
                    ofSetColor(255,255,255,255);
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    //erosionShader.End();
                    break;
                case 1:
                    
                    glEnable(GL_BLEND);
                    ofSetColor(255,255,255,255);
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    //ofRect(0,0,ofGetWidth(),ofGetHeight());
                    glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
                    ofRect(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case 2:
                    //DrawSlicedVersion(31);
                    
                    offsetPass->setEnabled(false);
                    colorizePass->setEnabled(true);
                    colorizePass->setVals((float)effectColor.r/255.0,(float)effectColor.g/255.0,(float)effectColor.b/255.0,(float)effectColor.midpoint/255.0,effectColor.ratio);
                    post.begin();
                    ofClear(0,0,0,1);
                    ofSetColor(255,255,255,255);
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    //mainImage[thisImage].draw(dispRegion.x,dispRegion.y,dispRegion.width,dispRegion.height);
                    post.end(false);
                    ofSetColor(255,255,255,255);
                    post.draw(0,0, ofGetWidth(), ofGetHeight());
                        break;
                        
                case 3:
                case 4:
                case 5:
                    
                    offsetFbo.begin();
                    ofBackground(127,127,127);
                    
                    if(video[useVideoIndicator].effect == 3){
                        for(int y=0;y<1024;y+=ofRandom(2,5)){
                            ofSetColor(ofRandom(100,156),ofRandom(120,130),255);
                            ofRect(0,y,1, 2);
                        }
                    }else if(video[useVideoIndicator].effect == 4){
                        for(int y=0;y<1024;y+=20){
                            ofSetColor(127+10.0*sin(y),127+20*cos((float)y/10.0f),127,255);
                            ofRect(0,y,1, 20);
                        }
                    }else if(video[useVideoIndicator].effect == 5){
                        for(int y=0;y<1024;y+=50){
                            ofSetColor(190,127,255);
                            //ofRect(y,y,50, 20);
                            ofTriangle(0, y, 1280, y-25, 1280, y+25);
                        }
                    }
                    
                    //
                    offsetFbo.end();
                    
                    offsetPass->setOffsetFboRef(&offsetFbo);
                    
                    offsetPass->setEnabled(true);
                    colorizePass->setEnabled(false);
                    post.begin();
                    ofClear(0,0,0,1);
                    ofSetColor(255,255,255,255);
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    post.end(false);
                    ofSetColor(255,255,255,255);
                    post.draw(0,0, ofGetWidth(), ofGetHeight());
                    
                    break;
                case 6:
                    
                    //offsetFboWide.begin();
                    //ofBackground(127,127,127);
                    {
                        float framenum = ofGetFrameNum();
                        float rate = framenumX2 / 10.0f;
                        float scale = noise.noise(rate / 6.0 );
                        //scale = scale * abs(scale);
                        if(scale<0)
                        {
                            unsigned char px[2048*3];
                            for(int i=0;i<2048;i++){
                                px[i*3+1] = 127+scale*(63.0*sin(rate/5.0f +(float)i/20.0)+63.0*sin(rate/3.0f +(float)i/10.0));
                                px[i*3+0] = 127;
                                px[i*3+2] = 255;
                                
                            }
                            ofTexture& tx = offsetFboWide.getTextureReference();
                            tx.loadData(px, 2048, 1, GL_RGB);
                            
                            
                            offsetPass->setOffsetFboRef(&offsetFboWide);
                        } else {
                            
                            unsigned char px[1024*3];
                            for(int i=0;i<1024;i++){
                                px[i*3+0] = 127+scale*(63.0*sin(rate/5.0f +(float)i/20.0)+63.0*sin(rate/3.0f +(float)i/10.0));
                                px[i*3+1] = 127;
                                px[i*3+2] = 255;
                                
                            }
                            ofTexture& tx = offsetFboTall.getTextureReference();
                            tx.loadData(px, 1, 1024, GL_RGB);
                            
                            
                            offsetPass->setOffsetFboRef(&offsetFboTall);
                        
                        }
                        
                        offsetPass->setEnabled(true);
                        colorizePass->setEnabled(false);
                        
                        post.begin();
                        ofClear(0,0,0,1);
                        ofSetColor(255,255,255,255);
                        currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                        post.end(false);
                        ofSetColor(255,255,255,255);
                        post.draw(0,0, ofGetWidth(), ofGetHeight());
                    }
                    
                    
                    break;
                case 7:
                    
                    startEdgeNoise();
                    
                    ofClear(0,0,0,1);
                    ofSetColor(255,255,255,255);
                    
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    
                    endEdgeNoise();
                    
                    offsetPass->setEnabled(true);
                    colorizePass->setEnabled(false);
                    
                    offsetPass->setOffsetFboRef(&edgeNoiseFboOffset);
                    
                    post.begin();
                    ofClear(0,0,0,1);
                    ofSetColor(255,255,255,255);
                    currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());
                    post.end(false);
                    ofSetColor(255,255,255,255);
                    post.draw(0,0, ofGetWidth(), ofGetHeight());
                    
                    break;
			} 
		}

	}





	
	switch(displayMode)
	{
	case 1://Fade
			ofSetColor(0,0,0,ofMap(framesSinceTrigger,0,10,0,255));
			ofRect(0,0,ofGetWidth(),ofGetHeight());
			break;
	case 2://16ths
		if(framesSinceTrigger%(framesPer4Beats/16)>0)
		{
			ofSetColor(0,0,0,255);
			ofRect(0,0,ofGetWidth(),ofGetHeight());
		}
		break;
	case 3://16ths
		if(framesSinceTrigger%(framesPer4Beats/16)>(framesPer4Beats/16-2))
		{
			ofSetColor(0,0,0,255);
			ofRect(0,0,ofGetWidth(),ofGetHeight());
		}
		break;
	case 4://8ths
		if(framesSinceTrigger%(framesPer4Beats/8)>2)
		{
			ofSetColor(0,0,0,255);
			ofRect(0,0,ofGetWidth(),ofGetHeight());
		}
		break;
	case 5://4ths
		if(framesSinceTrigger%(framesPer4Beats/4)>2)
		{
			ofSetColor(0,0,0,255);
			ofRect(0,0,ofGetWidth(),ofGetHeight());
		}
		break;
	case 0:
	default:
		break;
	}




	if(muteVid)
	{
		ofSetColor(0,0,0,255);
		ofRect(0,0,ofGetWidth(),ofGetHeight());
	}

	if(flashWhite)
	{
		ofSetColor(255,255,255,255);
		ofRect(0,0,ofGetWidth(),ofGetHeight());
		flashWhite=false;
	}

	if(OSCFade && OSCFramesSinceReceive<600) //if we lose connection, make sure we're not permanently muted!
	{
		ofSetColor(0,0,0,ofClamp(ofMap(OSCFadeLevel,0,1,-100,350),0,255));	
		ofRect(0,0,ofGetWidth(),ofGetHeight());	
	}
	if(OSCFramesSinceReceive<1000)
		OSCFramesSinceReceive++;

	float positionInVid = (float)video[useVideoIndicator].frameNum/video[useVideoIndicator].totalFramesX2;
    
	ofSetColor(255,255,255,40);
	ofRectRounded(10,ofGetHeight()-20,0,(ofGetWidth()-20),10,5,5,5,5);
	ofSetColor(255,255,255,ofClamp(ofMap(positionInVid,0.95,1,40,0),0,40));
	ofRectRounded(10,ofGetHeight()-20,0,positionInVid*(ofGetWidth()-20),10,5,5,5,5);

    float positionLoaded = fsLoader.loadingStatus();
    if(positionLoaded!=1.0f && positionLoaded!=0.0f ){
        
        ofSetColor(0,0,255,40);
        ofRectRounded(10,ofGetHeight()-40,0,(ofGetWidth()-20),10,5,5,5,5);
        ofSetColor(0,0,255,ofClamp(ofMap(positionLoaded,0.95,1,40,0),0,40));
        ofRectRounded(10,ofGetHeight()-40,0,positionLoaded*(ofGetWidth()-20),10,5,5,5,5);
        
    }


	//bool isMid = false;
	//float fraction = fmod(useFrame,1);
	//if( useFrame == (float)thisFrame)
	//{ //load up a new one and just use it
	//	ofSetColor(255,255,255,255);
	//	sprintf(filename,"%s\\%.6i.jpg",recordPath.c_str(), thisFrame);

	//} else
	//{//draw the last one full and the next one half
	//	ofSetColor(255,255,255,255);
	//	mainImage[1-thisImage].draw(0,0,ofGetWidth(),ofGetHeight(),40);	
	//	sprintf(filename,"%s\\%.6i.jpg",recordPath.c_str(), nextFrame);
	//	ofSetColor(255,255,255,128);
	//	mainImage[thisImage].draw(0,0,ofGetWidth(),ofGetHeight(),40);	

	//	
	//}

	//if(mainImage[thisImage].loadImage(filename))
	//	mainImage[thisImage].draw(0,0,ofGetWidth(),ofGetHeight(),40);	


	
	//lastFramenumX2 = framenumX2;

	lastVideoIndicator = useVideoIndicator;

	framesSinceTrigger++;

	//	glClearColor(0,0,0,0);
	//	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear( GL_DEPTH_BUFFER_BIT);
	//_flock.draw();
	
    ofPushMatrix();  
    ofTranslate(ofGetWidth()/2,ofGetHeight()/2,0);
  
   // box->Draw();
    
   //_flock->DrawTexture(&currentVidFBO);

    ofPopMatrix();
  
	if(writeToFile)
	{
		fboForWritingOutput.end();
		ofSetColor(255,255,255);
		ofPushMatrix();
		ofTranslate(0,ofGetHeight());
		ofScale(1,-1);
		fboForWritingOutput.draw(0,0,ofGetWidth(),ofGetHeight());
		ofPopMatrix();

		static int writeframenum = 0;
		if(writeframenum%2==0)
		{
			/*
			sprintf(filename,"out\\%.6i.bmp",writeframenum/2);
			ofImage im;
			im.setUseTexture(false);
			im.setFromPixels((unsigned char*)fboForWritingOutput.getPixels(),
				fboForWritingOutput.getWidth(),fboForWritingOutput.getHeight(),
				OF_IMAGE_COLOR);
			im.saveImage(filename);
*/
			sprintf(filename,"out\\%.6i.jpg",writeframenum/2);
			turboJpeg.save(&fboForWritingOutput,filename);
		}
		writeframenum++;
	}
	
	ofDrawBitmapString(ofToString(ofGetFrameRate()),10,10);
	ofDrawBitmapString(ofToString(video[videoIndicator].effect),10,20);
}

void GlitchPlayer::DrawSlicedVersion(int slices)
{
	ofSetColor(255,255,255,255);
	currentVidFBO.draw(0,0,ofGetWidth(),ofGetHeight());

	ofImage t;
	t.drawSubsection(0,0,5,5,1,1,4,4);

	//verticalSlices
	int sliceWidth = ofGetWidth()/slices;
	int sliceHeight = ofGetHeight();
	ofPushMatrix();
	ofTranslate(ofGetWidth(),ofGetHeight());
	ofScale(-1,-1);
	for(int i=0;i<slices;i++)
	{
		float top = 0;
		float bottom = ofGetHeight();
		float mid = sin(video[videoIndicator].sliceEffect.XX[i]*1000.0f+ofMap(video[videoIndicator].sliceEffect.YY[i],0,1,0.005,0.01)*video[videoIndicator].effectFrame);
		mid = ofMap(mid,-1,1,0,1)*ofGetHeight();
		if(i%2==0)
			top=mid;
		else
			bottom=mid;

		int thisx = i*ofGetWidth()/slices;
		int nextx = (i+1)*ofGetWidth()/slices;
		if(i==slices-1)
			nextx = ofGetWidth();
	/*	currentVidFBO.drawXY(
				thisx,top,
				nextx,top,
				nextx,bottom,
				thisx,bottom,
				thisx,top,
				nextx,top,
				nextx,bottom,
				thisx,bottom);*/
/*
		if(i==slices-1)//on the last one, lets extend out all the way to the edge to avoid missing the last little bit
			currentVidFBO.drawXY(
				i*sliceWidth,top,
				ofGetWidth(),top,
				ofGetWidth(),bottom,
				i*sliceWidth,bottom,
				i*sliceWidth,top,
				ofGetWidth(),top,
				ofGetWidth(),bottom,
				i*sliceWidth,bottom);		
		else
			currentVidFBO.drawXY(
				i*sliceWidth,top,
				(i+1)*sliceWidth,top,
				(i+1)*sliceWidth,bottom,
				i*sliceWidth,bottom,
				i*sliceWidth,top,
				(i+1)*sliceWidth,top,
				(i+1)*sliceWidth,bottom,
				i*sliceWidth,bottom);*/
	}
	ofPopMatrix();

	switch(speedIndicator)
	{
	case 0:
		video[videoIndicator].effectFrame-=5;
		break;
	case 1:
		video[videoIndicator].effectFrame-=3;
		break;
	case 2:
		video[videoIndicator].effectFrame-=1;
		break;
	case 3:
		video[videoIndicator].effectFrame+=1;
		break;
	case 4:
		video[videoIndicator].effectFrame+=3;
		break;
	case 5:
	default:
		video[videoIndicator].effectFrame+=5;
		break;
	}
	
}

//--------------------------------------------------------------
void GlitchPlayer::keyPressed  (int key){ 
	switch(key) {
		case 'l':
            //fsLoader.load("/Users/Josh/Media/IMG_2352.MOV","/Users/Josh/Media/GlitchPlayer/12");
            break;
		case ' ':
            ofToggleFullscreen();
            break;
        case '\t':
            if(keyboardMode == KMODE_VID)
                keyboardMode = KMODE_EFFECT;
            else
                keyboardMode = KMODE_VID;
            break;
            
    }
    
    if(keyboardMode == KMODE_VID){
        switch (key) {
            case '1':
                videoIndicator = 0;
                speedIndicator = 2;
                break;
            case 'q':
                videoIndicator = 0;
                speedIndicator = 3;
                break;
            case 'a':
                videoIndicator = 0;
                speedIndicator = 4;
                break;
            case 'z':
                videoIndicator = 0;
                speedIndicator = 5;
                break;
                
            case '2':
                videoIndicator = 1;
                speedIndicator = 2;
                break;
            case 'w':
                videoIndicator = 1;
                speedIndicator = 3;
                break;
            case 's':
                videoIndicator = 1;
                speedIndicator = 4;
                break;
            case 'x':
                videoIndicator = 1;
                speedIndicator = 5;
                break;
                
            case '3':
                videoIndicator = 2;
                speedIndicator = 2;
                break;
            case 'e':
                videoIndicator = 2;
                speedIndicator = 3;
                break;
            case 'd':
                videoIndicator = 2;
                speedIndicator = 4;
                break;
            case 'c':
                videoIndicator = 2;
                speedIndicator = 5;
                break;
        }
    }
    
    if(keyboardMode == KMODE_EFFECT){
        switch (key) {
            case '1':
                video[0].effectEngaged = !video[0].effectEngaged ;
                break;
            case 'q':
                video[0].effect = 3 ;
                break;
            case 'a':
                video[0].effect = 4 ;
                break;
            case 'z':
                video[0].effect = 7 ;
                break;
                
            case '2':
                video[1].effectEngaged = !video[1].effectEngaged ;
                break;
            case 'w':
                video[1].effect = 3 ;
                break;
            case 's':
                video[1].effect = 4 ;
                break;
            case 'x':
                video[1].effect = 6 ;
                break;
        }
    }

		
	//}
	movekeyResetFlag = true;
}

//--------------------------------------------------------------
void GlitchPlayer::keyReleased  (int key){ 

}

//--------------------------------------------------------------
void GlitchPlayer::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void GlitchPlayer::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void GlitchPlayer::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void GlitchPlayer::mouseReleased(){

}

void GlitchPlayer::startEdgeNoise(){
    
    
    //for(int i=0;i<4;i++){
    //    edgeNoiseEdgePass[i]->setEnabled(true);
    //}
    
    buildOffsetPass->setEnabled(true);
    buildOffsetPass->setInScale(0.1);//ofMap(mouseX,0,ofGetWidth(),0.0,1.0));
    buildOffsetPass->setOutScale(noise.noise((float)ofGetFrameNum()/10.0));//ofMap(mouseY,0,ofGetHeight(),0,0.5));
    buildOffsetPass->setMode(1);
    edgeNoiseConvPass->setEnabled(true);
    edgeNoiseProcessing.begin();
}

void GlitchPlayer::endEdgeNoise(){
    edgeNoiseProcessing.end(false);
    ofSetColor(255,255,255,255);
    edgeNoiseFboOffset.begin();
    edgeNoiseProcessing.draw(0,0, edgeNoiseFboOffset.getWidth(), edgeNoiseFboOffset.getHeight());
    edgeNoiseFboOffset.end();
    
}

void GlitchPlayer::initEdgeNoise(){
    
    edgeNoiseProcessing.init(1280,720);
    edgeNoiseProcessing.setFlip(false);
    //post.createPass<FxaaPass>()->setEnabled(false);
    
    for(int i=0;i<edgePassCount;i++){
        edgeNoiseEdgePass[i] = edgeNoiseProcessing.createPass<EdgePass>();
        edgeNoiseEdgePass[i]->setEnabled(false);
    }
    
    buildOffsetPass = edgeNoiseProcessing.createPass<BuildOffsetPass>();
    buildOffsetPass->setEnabled(false);
    
    edgeNoiseConvPass = edgeNoiseProcessing.createPass<ConvolutionPass>();
    edgeNoiseConvPass->setEnabled(false);
    
    ofFbo::Settings s;
    s.width = ofNextPow2(1280);//ofNextPow2(2048);
    s.height = ofNextPow2(720);//ofNextPow2(1080);
    s.textureTarget = GL_TEXTURE_2D;
    edgeNoiseFboEdges.allocate(s);
    edgeNoiseFboOffset.allocate(s);
}

