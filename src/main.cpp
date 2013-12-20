#include "ofMain.h"
#include "GlitchPlayer.h"

//========================================================================
int main( ){
	ofSetupOpenGL(1280/2,720/2,OF_WINDOW);			// <-------- setup the GL context
    
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new GlitchPlayer());

}
