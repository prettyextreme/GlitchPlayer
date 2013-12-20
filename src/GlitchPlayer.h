#ifndef _TEST_APP
#define _TEST_APP

//#define OF_ADDON_USING_OFXCUDAVID
#include "ofMain.h"
//#define OF_ADDON_USING_OFXOSC
//#define OF_ADDON_USING_OFXOPENCV
//#include "ofAddons.h"
#include "erosionShader.h"
//#include "movekeyShader.h"
//#include "ofxNoise.h"
//#include "CFlock.h"
#include "ofxMonomeControl.h"
#include "ofxPrettyExtremeUtils.h"
#include "ofxTurboJpeg.h"
#include "ofxPerlin.h"
#include "ofxPostProcessing.h"

#define BUFFER_SIZE 128

struct SLICEEFFECT{
	float XX[1000];
	float YY[1000];
	SLICEEFFECT()
	{
		for(int i=0;i<1000;i++)
		{
			XX[i]=ofRandom(0,1);
			YY[i]=ofRandom(0,1);
		}
	}
};

struct VIDEO{
	int totalFramesX2;
	int frameNum;
	int effect;
	std::string folder;
	int width;
	int height;
	int effectFrame;
	bool effectEngaged;
	SLICEEFFECT sliceEffect;
};

enum MONOME_MODE{
	PLAYBACK,
	EFFECT,
	COLOR
};

class GlitchPlayer : public ofBaseApp{
	
	public:
		
		void setup();
		void update();
		void draw();
		void exit();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased();

		void ToggleMonomeMode();

		ofImage mainImage[2];
		int thisImage;			

		
		//ofxOscReceiver	oscReceiver;
		//ofxOscSender	oscSender;

		int speedIndicator;
		int videoIndicator;

		VIDEO* video;
		int videosPresent;
		ofDirectory* dlist;

		bool flagRepress;

		int framesPer4Beats;
		int frameToStartCounting;
		int framesSinceTrigger;

		int displayMode;
		bool muteVid;
		bool flashWhite;
		bool skipForward;
		int skipAmount;//0-3

		bool OSCFade;
		float  OSCFadeLevel;
		int OSCFramesSinceReceive;

		MONOME_MODE monomeMode;

		ofFbo currentVidFBO;
		ofFbo accumulatorFBO[4];
		int			  accumulatorCurrent;
		ofFbo movekeyFBO[4];
		int			  movekeyCurrent;

		//Effects:
		
		void DrawSlicedVersion(int slices);

		//ErosionShader erosionShader;
		//MovekeyShader movekeyShader;
		bool movekeyResetFlag;

		int JitteryEngage; //0, 1 for hold, 2 for fadeoff
		int JitteryFrame;
		int FramesSinceJitteryHit;
		struct JSet{
			float scale;
			float left;
			float top;
			float rotation;
		} JitterySettings;

		int FramesSinceBumpPulse;

		ofxPerlin noise;

		EffectColor effectColor;
		void sendMonomeColors();

		ofFbo leftFBO;
		ofFbo rightFBO;


		//CFlock* _flock;



		ofFbo tinyFBO;
		//ofxCvGrayscaleImage gs[2];
		//ofxCvColorImage ci;
		int currentGS;

		ofxMonomeControl monomeControl;


		ofImage tempImg;

		ofFbo fboForWritingOutput;

		ofxTurboJpeg turboJpeg;

		bool totallyRandomFrame;
    
    
        //post processing effects
        ofxPostProcessing post;
        ColorizePass::Ptr colorizePass;
        OffsetPass::Ptr   offsetPass;
        ofFbo offsetFbo;
};

#endif
	
