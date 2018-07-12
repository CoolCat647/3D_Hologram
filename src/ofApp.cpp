// Code ref: https://blog.csdn.net/leixiaohua1020/article/details/12236091

#include "ofApp.h"

using namespace cv;
using namespace ofxCv;

//-------------------------- Parameter --------------------------
#define _USE_WEBCAM						// Uncomment this to use a live camera
#define _DEBUG_MODE						// Uncomment this to show camera image on screen
#define WEBCAM_ID 0

const static int THRESHOLD_OBJ_MIN_MOTION = 20;
const static int THRESHOLD_OBJ_MAX_MOTION = 200;
const static int TIME_COUNTDOWN = 6;

static string filename = "lofi-bunny.ply"; 	// "Shelves.3ds"; 
static int 		dis 		= 1000;		// Distance of EasyCam
static float 	scale 		= 0.6;		// Scale of 3D model
static int 		speed_self	= 2;		// Speed of motion
static int 		speed_fly	= 2;		// Speed of motion
static int  	self_rotate = 0;		// flag of self rotation
static int  	fly_rotate 	= 0;		// flag of fly rotation

static appState state = STATE_IDLE;		// To determine what the program state is.


//---------------------- Tracking variable ----------------------
IplImage *image = 0, *hsv = 0, *hue = 0, *mask = 0, *backproject = 0, *histimg = 0;
CvHistogram *hist = 0;

int backproject_mode = 0;
int select_object = 0;
int track_object = 0;
int show_hist = 1;
CvPoint origin;
CvRect selection;
CvRect track_window;
CvBox2D track_box;
CvConnectedComp track_comp;

CvPoint2D32f obj_center;		// To detect the motion
CvPoint2D32f pre_obj_center;

int hdims = 16;					// Bin number of histogram

float hranges_arr[] = {0,180};	// Range of histogram

float* hranges = hranges_arr;	// For initialization of histogram
int vmin = 10, vmax = 256, smin = 30;


//---------------------- Function Declare ----------------------
void cvSkinSegment(IplImage* img, IplImage* mask);
CvScalar hsv2rgb( float hue );


// --------------------- Global variables ----------------------
float startTime;


//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(15);			// Frame rate setting
	ofSetVerticalSync(true);
	ofToggleFullscreen();

	// EasyCam setup
	cam.setDistance(dis);

	// Font setup
	myfont.load("arial.ttf", 32);

	
	// Right	
	model1.loadModel(filename);
	model1.setPosition(300, 0 , 0);
	model1.setScale(scale, scale, scale);
	model1.setRotation(0, 0, 0, 0, 0);
   	model1.setRotation(1, 15, 0, 1, 0);
   	model1.setRotation(2, 90, 0, 0, 1);	

	// Left  	
   	model2.loadModel(filename);
	model2.setPosition(-300, 0 , 0);
	model2.setScale(scale, scale, scale);
   	model2.setRotation(0, 0, 1, 0, 0);
   	model2.setRotation(1, -15, 0, 1, 0);
   	model2.setRotation(2, -90, 0, 0, 1);

	// Top
   	model3.loadModel(filename);
	model3.setScale(scale, scale, scale);
	model3.setPosition(0, 300 , 0);
   	model3.setRotation(0,-15,1,0,0);
   	model3.setRotation(1,0,0,0,0);
   	model3.setRotation(2,180,0,0,1);

	// Down
   	model4.loadModel(filename);
	model4.setScale(scale, scale, scale);
	model4.setPosition(0, -300 , 0);
   	model4.setRotation(1,15,1,0,0);
   	model4.setRotation(2, 0,0,1,0);
   	model4.setRotation(3, 0,0,0,1);

	#ifdef _USE_WEBCAM
		vidGrabber.setDeviceID(WEBCAM_ID);
		vidGrabber.setVerbose(true);
		vidGrabber.setup(640,480);

		colorImg.allocate(640,480);
		grayImage.allocate(640,480);
	#endif

	state = STATE_IDLE;
}

//--------------------------------------------------------------


void ofApp::update(){
	#ifdef _USE_WEBCAM
		bool bNewFrame = false;

		vidGrabber.update();
		bNewFrame = vidGrabber.isFrameNew();

		if (bNewFrame) //bNewFrame
		{
			colorImg.setFromPixels(vidGrabber.getPixels());
			colorImgHSV = colorImg;
			colorImgHSV.convertRgbToHsv();
		}
	#endif

	if(speed_fly > 0) 	// for rotation decresion
		speed_fly--;
	if(speed_self > 0)
		speed_self--;

	int delta_x = obj_center.x - pre_obj_center.x;
	int delta_y = obj_center.y - pre_obj_center.y;

	if(delta_x > THRESHOLD_OBJ_MIN_MOTION && delta_x < THRESHOLD_OBJ_MAX_MOTION)
	{
		speed_self = std::abs(delta_x)  / 4;
		self_rotate	= 1;
	}
	else if(delta_x < -THRESHOLD_OBJ_MIN_MOTION && delta_x > -THRESHOLD_OBJ_MAX_MOTION)
	{
		speed_self = std::abs(delta_x) / 4;
		self_rotate = -1;
	}

	if(delta_y > THRESHOLD_OBJ_MIN_MOTION && delta_y < THRESHOLD_OBJ_MAX_MOTION)
	{
		speed_fly = std::abs(delta_y) / 4;
		fly_rotate	= 1;
	}
	else if(delta_y < -THRESHOLD_OBJ_MIN_MOTION && delta_y > -THRESHOLD_OBJ_MAX_MOTION)
	{
		speed_fly = std::abs(delta_y) / 4;
		fly_rotate = -1;
	}

	pre_obj_center = obj_center;
}

//--------------------------------------------------------------
ofImage myOfImage;
void ofApp::draw(){
	ofBackgroundGradient(ofColor(64), ofColor(0));
	
	//myOfImage.allocate(colorImg.getWidth(), colorImg.getHeight(), OF_IMAGE_COLOR);
	cam.begin();
	cam.setDistance(dis);

	switch(state)
	{
		case STATE_IDLE:
			startTime = ofGetElapsedTimeMillis();
			state = STATE_CALI;
			break;

		case STATE_CALI:
			if( !ofApp::camCalibration())	//return 0 when calibrating successfully
			{
				state = STATE_TRAC;
				startTime = ofGetElapsedTimeMillis();
			}
			break; 

		case STATE_TRAC:
			glRotatef(180, 0, 1, 0);
			ofApp::camTracking();
			
			//image.draw(50,50)
			ofApp::modelDisplay();
			
			break;

		default:
			cout << "STATE ERROR!" << endl;
			ofApp::setup();
			break;
	}
	
	cam.end();
}

bool ofApp::camCalibration(void)
{
	static char str[30];
	float timer = ofGetElapsedTimeMillis() - startTime;
	int radius_cali = 150;
	int pos_x = -320,
		pos_y = -480;

	glRotatef(180, 0, 0, 1);
	
	colorImg.draw(pos_x , pos_y);
	ofNoFill();
	ofSetColor(ofColor::red);
	//ofDrawCircle(pos_x + radius_cali, -radius_cali, radius_cali);
	ofDrawRectangle(pos_x + radius_cali, -radius_cali, radius_cali, radius_cali);

	ofSetColor(ofColor::white);

	if(timer > TIME_COUNTDOWN * 1000)	// The last one
	{
		myfont.drawString("  Calibration\nCountDown 0!", -32*5, -500);
		selection.x = 0;
		selection.y = 0;
		selection.width = radius_cali;
		selection.height = radius_cali;
		track_object = -1;
		pre_obj_center.x = obj_center.x = 0;
		pre_obj_center.y = obj_center.y = 0;
		return 0;
	}

	for(int i = TIME_COUNTDOWN - 1; i > 0; i--)
	{
		if(timer > i * 1000)
		{
			sprintf(str,"Calibration\nCountDown %d", TIME_COUNTDOWN - i);
			myfont.drawString(str, -32*5, -480);
			break;
		}
	}
	/*if(timer > 6000){
		
	}
	else if(timer > 5000){
		myfont.drawString("  Calibration\nCountDown 1!", -32*5, -500);
	}
	else if(timer > 4000){
		myfont.drawString("  Calibration\nCountDown 2!", -32*5, -500);
	}
	else if(timer > 3000){
		myfont.drawString("  Calibration\nCountDown 3!", -32*5, -500);
	}
	else if(timer > 2000){
		myfont.drawString("  Calibration\nCountDown 4!", -32*5, -500);
	}
	else if(timer > 1000){
		myfont.drawString("  Calibration\nCountDown 5!", -32*5, -500);
	}
	else if(timer > 0){
		myfont.drawString("  Calibration\nCountDown 6!", -32*5, -500);
	}*/

	return 1;
}

void ofApp::modelDisplay(void)
{
   	ofSetColor(ofColor::white);
   	unsigned int cnt1 = model1.getNumRotations();
   	model1.setRotation(cnt1, speed_fly, fly_rotate, 0, 0);
   	model1.setRotation(cnt1+1, speed_self, 0, self_rotate, 0);
   	model1.draw(OF_MESH_WIREFRAME); // same as model.drawWireframe();
   	
   	ofSetColor(224,255,255);	//Color LightCyan
   	model2.setRotation(cnt1, speed_fly, fly_rotate, 0, 0);
   	model2.setRotation(cnt1+1, speed_self, 0, self_rotate, 0);
   	model2.draw(OF_MESH_WIREFRAME);  	
 
   	ofSetColor(238,130,238);	//Color Violet
   	model3.setRotation(cnt1, speed_fly, fly_rotate, 0, 0);
   	model3.setRotation(cnt1+1, speed_self, 0, self_rotate, 0);
   	model3.draw(OF_MESH_WIREFRAME);

   	ofSetColor(255,215,0);		//Color Gold
   	model4.setRotation(cnt1, speed_fly, fly_rotate, 0, 0);
   	model4.setRotation(cnt1+1, speed_self, 0, self_rotate, 0);
   	model4.draw(OF_MESH_WIREFRAME);
}




void ofApp::camTracking(void)
{
    IplImage* frame = 0;
    int i, bin_w;

    frame = colorImg.getCvImage();
    if( !frame )
        return;

    if( !image )
    // At beginning, the image is not ready yet for hand detection function.
    // Do initialization 
    {
        image = cvCreateImage( cvGetSize(frame), 8, 3 );
        image->origin = frame->origin;
        hsv = cvCreateImage( cvGetSize(frame), 8, 3 );
        hue = cvCreateImage( cvGetSize(frame), 8, 1 );
        mask = cvCreateImage( cvGetSize(frame), 8, 1 );			//分配掩膜图像空间
        backproject = cvCreateImage( cvGetSize(frame), 8, 1 ); //分配反向投影图空间,大小一样,单通道
        hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
       
        histimg = cvCreateImage( cvSize(320,200), 8, 3 );	// For display histogram
        
        cvZero( histimg );						// Set the background color of histogram image to black
    }

    cvCopy( frame, image, 0 );
    cvCvtColor( image, hsv, CV_RGB2HSV );		// Convert RGB image to HSV image

    if( track_object != 0)
    // if the track_object is nonzero, it represent there are some object need to be track
    {
        int _vmin = vmin, _vmax = vmax;

        // Create mask image, the range of mask is H：0~180，S：smin~256，V：vmin~vmax
        cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),
                    cvScalar(180,256,MAX(_vmin,_vmax),0), mask );
        
        cvSplit( hsv, hue, 0, 0, 0 );			// Split H conponent 
		
		
        if( track_object < 0 )
        // if the feature of the object which need to be detect isn't extracted, do feature extracting
        {
            float max_val = 0.f;

            // assign the default rectagle for ROI
            cvSetImageROI( hue, selection );
            
            cvSetImageROI( mask, selection );		//设置掩膜板选择框为ROI
            cvCalcHist( &hue, hist, 0, mask );		//得到选择框内且满足掩膜板内的直方图
            cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
            cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );	// 对直方图的数值转为0~255
            cvResetImageROI( hue );					// Remove ROI
            cvResetImageROI( mask );				// Remove ROI
            track_window = selection;
            track_object = 1;						// Set track_object flag to 1
            cvZero( histimg );
            bin_w = histimg->width / hdims;
            for( i = 0; i < hdims; i++ )
            // Draw the histogram result
            {
                int val = cvRound( cvGetReal1D(hist->bins,i)*histimg->height/255 );
                CvScalar color = hsv2rgb(i*180.f/hdims);
                cvRectangle( histimg, cvPoint(i*bin_w,histimg->height),
                             cvPoint((i+1)*bin_w,histimg->height - val),
                             color, -1, 8, 0 );
            }
        }

        cvCalcBackProject( &hue, backproject, hist );	//计算hue的反向投影图
        cvAnd( backproject, mask, backproject, 0 );		//得到掩膜内的反向投影

        // Use the 'MeanShift' algorithm to tracking object which has been assigned,
        // and return the tracking result
        cvCamShift( backproject, track_window,
                    cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                    &track_comp, &track_box );
        

        track_window = track_comp.rect;
        // Get the tracking window

        obj_center = track_box.center;
        
        #ifdef _DEBUG_MODE
	        if( backproject_mode )
	            cvCvtColor( backproject, image, CV_GRAY2BGR );//CV_GRAY2BGR
	        cvCvtColor( image, image, CV_BGR2RGB );
	            
	        if( image->origin )
	            track_box.angle = -track_box.angle;

	        // Draw the block on the object which need to be detect
	        cvEllipseBox( image, track_box, CV_RGB(255,0,0), 3, CV_AA, 0 );
	        
	        colorImgHSV = image;
	        colorImgHSV.draw(0,0);
	    #endif
    }
    #ifdef _DEBUG_MODE
		if( select_object && selection.width > 0 && selection.height > 0 )
		
		{
			cvSetImageROI( image, selection );
			cvXorS( image, cvScalarAll(255), image, 0 );
			cvResetImageROI( image );
		}
		cvShowImage( "CamShiftDemo", image );
	    cvShowImage( "Histogram", histimg );
	#endif
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key){
	case '1':
		speed_self--;
		speed_fly--;
		break;
	case '2':
		speed_self++;
		speed_fly++;
		break;
	case '3':
		speed_self = speed_fly= 3;
		break;
	case 'a':
		fly_rotate 	= 0;
		self_rotate	= 0;
		break;
	case 's':
		fly_rotate	= 0;
		self_rotate	= 1;
		break;
	case 'd':
		fly_rotate	=1;
		self_rotate	=0;
		break;
	case '-':
		dis = dis + 200;
		break;
	case '=':
		dis = dis - 200;
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

CvScalar hsv2rgb( float hue )
//用于将Hue量转换成RGB量
{
    int rgb[3], p, sector;
    static const int sector_data[][3]=
        {{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
    hue *= 0.033333333333333333333333333333333f;
    sector = cvFloor(hue);
    p = cvRound(255*(hue - sector));
    p ^= sector & 1 ? 255 : 0;
 
    rgb[sector_data[sector][0]] = 255;
    rgb[sector_data[sector][1]] = 0;
    rgb[sector_data[sector][2]] = p;
 
    return cvScalar(rgb[2], rgb[1], rgb[0],0);
}

//--------------------------------------------------------------
void cvSkinSegment(IplImage* img, IplImage* mask){  
	CvSize imageSize = cvSize(img->width, img->height);  
	IplImage *imgY = cvCreateImage(imageSize, IPL_DEPTH_8U, 1);  
	IplImage *imgCr = cvCreateImage(imageSize, IPL_DEPTH_8U, 1);  
	IplImage *imgCb = cvCreateImage(imageSize, IPL_DEPTH_8U, 1);  


	IplImage *imgYCrCb = cvCreateImage(imageSize, img->depth, img->nChannels);  
	cvCvtColor(img,imgYCrCb,CV_RGB2YCrCb);  
	cvSplit(imgYCrCb, imgY, imgCr, imgCb, 0);  
	int y, cr, cb, l, x1, y1, value;  
	unsigned char *pY, *pCr, *pCb, *pMask;  

	pY = (unsigned char *)imgY->imageData;  
	pCr = (unsigned char *)imgCr->imageData;  
	pCb = (unsigned char *)imgCb->imageData;  
	pMask = (unsigned char *)mask->imageData;  
	cvSetZero(mask);  
	l = img->height * img->width;  
	for (int i = 0; i < l; i++){  
		y  = *pY;  
		cr = *pCr;  
		cb = *pCb;  
		cb -= 109;  
		cr -= 152;  

		x1 = (819*cr-614*cb)/32 + 51;  
		y1 = (819*cr+614*cb)/32 + 77;  
		x1 = x1*41/1024;  
		y1 = y1*73/1024;  
		value = x1*x1+y1*y1;

		if(y<100)    (*pMask)=(value<700) ? 255:0;   //700
		else        (*pMask)=(value<850)? 255:0;  
		pY++;  
		pCr++;  
		pCb++;  
		pMask++;  
	}  
	cvReleaseImage(&imgY);  
	cvReleaseImage(&imgCr);  
	cvReleaseImage(&imgCb);  
	cvReleaseImage(&imgYCrCb);  
}
