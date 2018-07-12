#pragma once

#include <string.h>
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"

/* 3D model library */
#include "ofxAssimpModelLoader.h"
#include "ofVboMesh.h"

#define _USE_LIVE_VIDEO		// uncomment this to use a live camera

typedef enum{
  STATE_IDLE = -1,
  STATE_CALI = 0, // Calibration state
  STATE_TRAC = 1 // Tracking state


} appState;

class ofApp : public ofBaseApp{
	private:

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		bool camCalibration(void);
		void camTracking(void);
		void modelDisplay(void);

		ofTrueTypeFont myfont;
		
		ofEasyCam cam;
		ofxAssimpModelLoader model1;
		ofxAssimpModelLoader model2;
		ofxAssimpModelLoader model3;
		ofxAssimpModelLoader model4;

		ofVideoGrabber 		vidGrabber;

		ofxCvColorImage			colorImg;
		ofxCvColorImage			colorImgHSV;

        ofxCvGrayscaleImage 	grayImage;

        ofxCvContourFinder 	contourFinder;
};
