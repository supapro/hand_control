#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

using namespace std;
using namespace cv;

#define DIRECTION_RIGHT 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3
#define DIRECTION_UP 4
#define DIRECTION_NONE 5

#define Q_Key 1048689
#define C_Key 1048675
#define M_Key 1048685
#define G_Key 1048679
#define Enrer_Key 1048589

#define Min_dir_frames 20

VideoCapture cap(0);
double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
Mat frame; // cap >> frame;
Rect box(1,1,1,1); // hand recognition area

Scalar lower = CvScalar(0, 0, 153); // hsv ranges of color filter
Scalar upper = CvScalar(26,110,256);

bool gestures = false; // recognize and process gestures?
bool click = false; // process clicks?
bool mouse = false; // control mouse?

// xLib
Display* dpy = XOpenDisplay(0);
int scr = XDefaultScreen(dpy);
Window root_window = XRootWindow(dpy, scr);
int winHeight = DisplayHeight(dpy, scr);
int winWidth  = DisplayWidth(dpy, scr);
float m = (float)winHeight/(float)winWidth;

CvSize size(340, 220); // size for frame resizing
Point higherPoint(width, height); // setting the start position at the bottom
int minArea = 40*40;
int maxArea = 200*200;

bool doneHSV = false;
void on_trackbar(int, void *) {}
void adjustHSV(){ // adjust hsv ranges of color filter
    int H_MIN = lower[0];
    int S_MIN = lower[1];
    int V_MIN = lower[2];
    int H_MAX = upper[0];
    int S_MAX = upper[1];
    int V_MAX = upper[2];
    string winName = "hsv adjust";
    namedWindow(winName, 0);
    char TrackbarName[50];
    sprintf(TrackbarName, "H_MIN", H_MIN);
    sprintf(TrackbarName, "H_MAX", H_MAX);
    sprintf(TrackbarName, "S_MIN", S_MIN);
    sprintf(TrackbarName, "S_MAX", S_MAX);
    sprintf(TrackbarName, "V_MIN", V_MIN);
    sprintf(TrackbarName, "V_MAX", V_MAX);
    createTrackbar("H_MIN", winName, &H_MIN, 256, on_trackbar);
    createTrackbar("H_MAX", winName, &H_MAX, 256, on_trackbar);
    createTrackbar("S_MIN", winName, &S_MIN, 256, on_trackbar);
    createTrackbar("S_MAX", winName, &S_MAX, 256, on_trackbar);
    createTrackbar("V_MIN", winName, &V_MIN, 256, on_trackbar);
    createTrackbar("V_MAX", winName, &V_MAX, 256, on_trackbar);
    namedWindow("hsv mask");
    Mat hsv, mask;
    while (!doneHSV)
    {
        cap >> frame;
        flip(frame, frame, CV_CVTIMG_FLIP);
        Mat im(frame, box);
        cvtColor(im, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), mask);
        imshow("hsv mask", mask);
        int key = waitKeyEx(1);
        if (key == Enrer_Key){ // end adjusting and apply changes if pressed "enter"
                doneHSV = true;
        }
        if(key == Q_Key){ // return if pressed "q"
                          // lower amd upper do not change
            doneHSV = true;
            destroyWindow(winName);
            destroyWindow("hsv mask");
            return;
        }
    }
    lower = Scalar(H_MIN, S_MIN, V_MIN);
    upper = Scalar(H_MAX, S_MAX, V_MAX);
    cout << "Lower: " << H_MIN << ", " << S_MIN << ", " << V_MIN << endl;
    cout << "Upper: " << H_MAX << ", " << S_MAX << ", " << V_MAX << endl;
    destroyWindow(winName);
    destroyWindow("hsv mask");
}

bool doneBox = false;
static void drawBox(int event, int x, int y, int, void *){
      if (doneBox)
        return;
      switch (event)
      {
      case CV_EVENT_LBUTTONUP:
            doneBox = true;
            break;
      case CV_EVENT_LBUTTONDOWN:
            box.x = x;
            box.y = y;
            break;
      case CV_EVENT_MOUSEMOVE:
            box.width = x;
            box.height = y;
            break;
      }
}
// adjust hand recognition area
// press left mouse button for change box start point
// right mouse button - finish adjust and apply changes
void adjustBox(){
      namedWindow("select capture box", CV_WINDOW_AUTOSIZE);
      while (!doneBox)
      {
            cap >> frame;
            flip(frame, frame, CV_CVTIMG_FLIP);
            setMouseCallback("select capture box", drawBox);
            rectangle(frame, box, Scalar(0, 0, 255, 0), 2);
            imshow("select capture box", frame);
            int key = waitKeyEx(1);
            if (key == Enrer_Key){ // end adjusting and apply changes if pressed "enter"
                  doneBox = true;
            }
      }
      destroyWindow("select capture box");
}

void configure(){
    adjustBox();
    adjustHSV();
}

void mouseClick(int button){
    Display *display = XOpenDisplay(NULL);
    XEvent event;
    if(display == NULL){
        fprintf(stderr, "Cannot initialize the display\n");
        exit(EXIT_FAILURE);
    }
    memset(&event, 0x00, sizeof(event));
    event.type = ButtonPress;
    event.xbutton.button = button;
    event.xbutton.same_screen = True;
    XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }
    if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) fprintf(stderr, "Error\n");
    XFlush(display);
    usleep(100000);
    event.type = ButtonRelease;
    event.xbutton.state = 0x100;

    if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) fprintf(stderr, "Error\n");
    XFlush(display);
    XCloseDisplay(display);
}

XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers){
   XKeyEvent event;
   event.display     = display;
   event.window      = win;
   event.root        = winRoot;
   event.subwindow   = None;
   event.time        = CurrentTime;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = True;
   event.keycode     = XKeysymToKeycode(display, keycode);
   event.state       = modifiers;
   if(press)
      event.type = KeyPress;
   else
      event.type = KeyRelease;
   return event;
}

void pressKey(int keyCode){
    Display *display = XOpenDisplay(NULL);
    if(display == NULL){
        fprintf(stderr, "Cannot initialize the display\n");
        exit(EXIT_FAILURE);
    }
    Window winRoot = XDefaultRootWindow(display);
    Window winFocus;
    int    revert;
    XGetInputFocus(display, &winFocus, &revert);
    XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, keyCode, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
    event = createKeyEvent(display, winFocus, winRoot, false, keyCode, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
    XCloseDisplay(display);
}

void mouse_control_pos(Point higherPoint){
    int mx = winWidth / box.width;
    int my = winHeight / box.height;
    XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, higherPoint.x * mx, higherPoint.y * my);
    XFlush(dpy);
}

void mouse_control_click(int changedBrWidth, bool brWidthChanged,
                    int lastBrWidth, int changedBrWidthFrames){
    brWidthChanged = false;
    if(changedBrWidth - 15 > lastBrWidth && changedBrWidthFrames > 20 && click){
        changedBrWidthFrames = 0;
        brWidthChanged = false;
        mouseClick(Button1);
    }
}

int process_gesture(map<int, int> direction){
    if(direction[DIRECTION_RIGHT] > Min_dir_frames){
        pressKey(XK_Right);
        cout << "right" << endl;
        return DIRECTION_RIGHT;
    }
    if(direction[DIRECTION_LEFT] > Min_dir_frames){
        pressKey(XK_Left);
        cout << "left" << endl;
        return DIRECTION_LEFT;
    }
    if(direction[DIRECTION_UP] > Min_dir_frames*2){
        pressKey(XK_Escape);
        cout << "up" << endl;
        return DIRECTION_UP;
    }
    if(direction[DIRECTION_NONE] > Min_dir_frames*2){
        pressKey(XK_space);
        cout << "none" << endl;
        return DIRECTION_NONE;
    }
    return 0;
}

int recognize_gesture(map<int, int> direction,
                    vector<Point> points, Point higherPoint){
    Point p = points.at(points.size()-2);
    if(p.x + 10 < higherPoint.x ){
        return DIRECTION_RIGHT;
    }   
    if(p.x - 10 > higherPoint.x){
        return DIRECTION_LEFT;
    }
    if(p.y + 5 < higherPoint.y){
        return DIRECTION_DOWN;
    }
    if(p.y - 5 > higherPoint.y){
        return DIRECTION_UP;
    }
    if(p.y-higherPoint.y  < 5 &&
        p.x-higherPoint.x < 5){
        return DIRECTION_NONE;
    }
    return 0;
}

pair<Mat, Mat> prepare_frame(Mat frame){
    Mat img, blur;
    flip(frame, frame, CV_CVTIMG_FLIP);
    rectangle(frame, box, Scalar(0, 0, 255, 0), 2);
    img = Mat(frame, Rect(box.x + 2, box.y + 2, box.width - 4, box.height - 4));
    resize(img, img, size);
    GaussianBlur(img, blur, Size(25, 25), 0, 0);
    medianBlur(blur, blur, 5);
    Mat imgHSV = Mat(size.height, size.width, 8, 3);
    cvtColor(blur, imgHSV, COLOR_BGR2HSV);
    return make_pair(img, imgHSV);
}

Mat mask_morph(Mat mask){
    Mat maskMorph;
    Mat erodeElement = getStructuringElement( MORPH_RECT,Size(3,3));
	Mat dilateElement = getStructuringElement( MORPH_RECT,Size(8,8));
    erode(mask,maskMorph,erodeElement);
    erode(mask,maskMorph,erodeElement);
    dilate(mask,maskMorph,dilateElement);
    dilate(mask,maskMorph,dilateElement);
    return maskMorph;
}

int main(int argc, char **argv){
    configure();
    int lastBrWidth = 0;
    int changedBrWidth = 0;
    int changedBrWidthFrames = 0;
    bool brWidthChanged = false;
    bool directionRecognized = false;
    map<int, int> direction;
    vector<Point> points;
    Mat img, mask, maskMorph, boxFrame, imgHSV;
    while(true){
        directionRecognized = false;
        cap >> frame;
        tie(img, imgHSV) = prepare_frame(frame);
        inRange(imgHSV, lower, upper, mask);
        maskMorph = mask_morph(mask);

        int x,y;
        vector< vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(maskMorph,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );
        vector<vector<Point> >hull(contours.size());   
        vector<vector<int> > hullI(contours.size());
        vector<vector<Vec4i>> defects(contours.size());
        int dirId = 0;
        if (hierarchy.size() > 0) {
            for (int i = 0; i >= 0; i = hierarchy[i][0]) {
                Moments moment = moments((cv::Mat)contours[i]);
                double area = moment.m00;
                if(area>minArea&&area<maxArea){
                    convexHull(contours[i], hull[i], false);
                    convexHull(contours[i], hullI[i], false);
                    if(hullI[i].size() < 3)
                        continue;
                    convexityDefects(contours[i], hullI[i], defects[i]);
                    int j = 0;
                    higherPoint.x = 0;
                    higherPoint.y = height;
                    Point ptStart, ptEnd, ptFar;
                    for(const Vec4i& v : defects[i]){
                        float depth = v[3] / 256;
                        if (depth < 11)
                            continue;
                        int startidx = v[0];ptStart = Point(contours[i][startidx]);
                        int endidx = v[1];ptEnd = Point(contours[i][endidx]);
                        int faridx = v[2];ptFar = Point(contours[i][faridx]);
                        line(img, ptStart, ptEnd, Scalar(0, 255, 0), 2);
                        line(img, ptStart, ptFar, Scalar(0, 255, 0), 2);
                        line(img, ptEnd, ptFar, Scalar(0, 255, 0), 3);
                        circle(img, ptFar, 5, Scalar(255, 255, 255), 2);
                        Scalar color(0, 0, 0);
                        if(ptEnd.y < higherPoint.y){
                            higherPoint.y = ptEnd.y;
                            higherPoint.x = ptEnd.x;
                        }
                        circle(img, ptEnd, 10, Scalar(0, 0, 0), 2);
                    }         
                    if(higherPoint.y != 0 && higherPoint.x != 0){
                        points.push_back(Point(higherPoint.x, higherPoint.y));                        
                        circle(img, Point(higherPoint.x, higherPoint.y), 10, Scalar(0, 0, 255), 2);
                    }
                    x = moment.m10/area;
                    y = moment.m01/area;
                    Rect br = boundingRect(contours[i]);
                    if (br.width < 30 || br.height < 30) {
                     continue;   
                    }
                    if(lastBrWidth != br.width) {
                        lastBrWidth = changedBrWidth;
                        changedBrWidth = br.width;                        
                        brWidthChanged = true;
                        changedBrWidthFrames++;
                    }
                    drawContours(img, hull, i, Scalar(0, 255, 255), 2, 8, hierarchy, 0, Point(1, 1));
                    circle(img, Point(x, y), 3, Scalar(255, 0, 0));
                    drawContours(img, contours, i, Scalar(255, 255, 0, 0), 1, 8, hierarchy, 0, Point(1, 1));
                    rectangle(img, br, Scalar(0, 0, 255));
                }
            }
        }
        if(gestures && higherPoint.y != 0 && higherPoint.x != 0){
            dirId = recognize_gesture(direction, points, higherPoint);
            if(dirId != 0){
                direction[dirId] += 1;
                directionRecognized = true;
            }
        }
        if(mouse)
            mouse_control_pos(higherPoint);
        if(click)
            mouse_control_click(changedBrWidth, brWidthChanged, 
                                lastBrWidth, changedBrWidthFrames);
        if(points.size() == 0){
            direction.clear();
            points.clear();
        }
        if(gestures && directionRecognized){
            dirId = process_gesture(direction);
            if(dirId != 0){
                direction.clear();
                points.clear();
                //sleep(1);
                usleep(10000);
            }
        }
        // Showing
        imshow("result", img);
        imshow("mask", mask);
        imshow("mask morph", maskMorph);
        // Processing pressed key
        int key = waitKeyEx(1);
        if (key != -1)
            printf("Key presed: %i\n", key);
        switch (key){
            case Q_Key: // exit
                return 0;
            case C_Key: // enable/disable clicking
                click = !click;
                cout << "clicking " << (click ? "enabled" : "disabled") << endl;
                break; 
            case M_Key: // enable/disable mouse
                mouse = !mouse;
                cout << "mouse " << (mouse ? "enabled" : "disabled") << endl;
                break;
            case G_Key: // enable/disable gestures
                gestures = !gestures;
                cout << "gestures " << (gestures ? "enabled" : "disabled") << endl;
                break;
        }
    }
    return 0;
}
