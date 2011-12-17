#if IBM
#include <windows.h>
#endif
#if LIN
#include <GL/gl.h>
#else
#if __GNUC__
#include <GL/gl.h>
#else
#include <gl.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "XPLMDisplay.h"
#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMCamera.h"
#include "xplane.h"
#include "settings.h"
#include "colomboard.h"
#include "properties.h"
#include "log.h"
#include "thread.h"

XPLMDataRef g_nav1_standby_frequency_hz;
int g_nav1_standby_frequency_hz_prev = 0;
XPLMDataRef g_comfreq;
int g_comfreq_prev = 0;
XPLMDataRef g_flap_ratio;
float g_flap_ratio_prev = 0.0;
int initialized = 0;
int deviceInitialized = 0;
char * filename = "Resources/plugins/flapindicator.cfg";
int flap_degree[101];
char dataRefString[MAXVALUESIZE] = { "" };
int countUSBInit = 0;
int countDataRefInit = 0;
pthread_t g_thread;
int g_thread_id = 1;
int g_thread_return_code = 0;
struct shared_data g_shared_data;

/*
 * Global Variables.  We will store our single window globally.  We also record
 * whether the mouse is down from our mouse handler.  The drawing handler looks
 * at this information and draws the appropriate display.
 *
 */

XPLMWindowID gWindow = NULL;
int gClicked = 0;
int MyDrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void * inRefcon);

void MyDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon);

void MyHandleKeyCallback(XPLMWindowID inWindowID, char inKey,
		XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon,
		int losingFocus);

int MyHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y,
		XPLMMouseStatus inMouse, void * inRefcon);

/*
 * XPluginStart
 *
 * Our start routine registers our window and does any other initialization we
 * must do.
 *
 */
PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
	/* First we must fill in the passed in buffers to describe our
	 * plugin to the plugin-system. */

	strcpy(outName, "X-Plane Schweiz - Flapindicator");
	strcpy(outSig, "colomboard.flapindicator");
	strcpy(outDesc, "colomboard 10.07.31");

	gWindow = XPLMCreateWindow(50, 300, 100, 200, /* Area of the window. */
	1, /* Start visible. */
	MyDrawWindowCallback, /* Callbacks */
	MyHandleKeyCallback, MyHandleMouseClickCallback, NULL); /* Refcon - not used. */

	/* We must return 1 to indicate successful initialization, otherwise we
	 * will not be called back again. */
	initLog();

	return 1;
}

/*
 * XPluginStop
 *
 * Our cleanup routine deallocates our window.
 *
 */
PLUGIN_API void XPluginStop(void) {
	XPLMDestroyWindow(gWindow);
}

/*
 * XPluginDisable
 *
 * We do not need to do anything when we are disabled, but we must provide the handler.
 *
 */
PLUGIN_API void XPluginDisable(void) {
	g_shared_data.stop = 1;
	closeDevice();
#ifdef __linux__
	sleep(50);
#else
	Sleep(500);
#endif
	cleanup(); //please return my resources

}

/*
 * XPluginEnable.
 *
 * We don't do any enable-specific initialization, but we must return 1 to indicate
 * that we may be enabled at this time.
 *
 */
PLUGIN_API int XPluginEnable(void) {
	int i = 0;
	int currentFlapValue = 4500;
	char cTmp[256];
	writeLog("loading settings.\n");
	loadSettings();
	writeLog("initialize usb device.\n");
	deviceInitialized = initDevice();
	if (deviceInitialized >= 0) {
		writeLog("usb device successfully initialized.\n");
	}

	char searchValue[MAXVALUESIZE] = { "" };

	sprintf(cTmp, "load propertyfile: %s\n", filename);
	writeLog(cTmp);
	loadPropertyFile(filename); //open the pandora's box
	loadProperties(); //load the props from the file
	listProperties(); //list all the properties ya got
	//get me the property count
	sprintf(cTmp, "Property Count        : %d\n", getPropertyCount());
	writeLog(cTmp);
	getProperty("flapindicator_dataref", &dataRefString[0]);
	sprintf(cTmp, "Value for flapindicator_dataref   : %s\n", dataRefString);
	writeLog(cTmp);
	for (i = 0; i < 101; i++) {
		sprintf(cTmp, "pos_%d", i);
		getProperty(cTmp, &searchValue[0]);
		if (strlen(searchValue) > 0) {
			flap_degree[i] = atoi(searchValue);
			currentFlapValue = flap_degree[i];
		} else {
			flap_degree[i] = currentFlapValue;
		}
		sprintf(cTmp, "Value for pos_%d: %d\n", i, currentFlapValue);
		writeLog(cTmp);
	}

	// init datarefs
	g_nav1_standby_frequency_hz = XPLMFindDataRef(
			"sim/cockpit2/radios/actuators/nav1_standby_frequency_hz");
	g_nav1_standby_frequency_hz_prev
			= XPLMGetDatai(g_nav1_standby_frequency_hz);
	g_comfreq = XPLMFindDataRef("sim/cockpit/radios/com1_freq_hz");
	g_comfreq_prev = XPLMGetDatai(g_comfreq);
	// "sim/flightmodel2/controls/flap1_deploy_ratio");
	// "x737/systems/flaps/te_ext_all_left"
	sprintf(cTmp, "searching DataRef: %s\n", dataRefString);
	writeLog(cTmp);
	g_flap_ratio = XPLMFindDataRef(dataRefString);
	if (g_flap_ratio == NULL) {
		sprintf(cTmp, "Could not find DataRef %s.\n", dataRefString);
		writeLog(cTmp);
	} else {
		sprintf(cTmp, "DataRef %s found.\n", dataRefString);
		writeLog(cTmp);
		g_flap_ratio_prev = XPLMGetDataf(g_flap_ratio);
	}

	g_shared_data.thread_id = g_thread_id;
	g_shared_data.stop = 0;
	g_thread_return_code = pthread_create(&g_thread, NULL, run,
			(void *) &g_shared_data);
	if (g_thread_return_code) {
		sprintf(cTmp,
				"Colomboard error: return code from pthread_create() is %d\n",
				g_thread_return_code);
		writeLog(cTmp);
		return 0;
	}

	return 1;
}

/*
 * XPluginReceiveMessage
 *
 * We don't have to do anything in our receive message handler, but we must provide one.
 *
 */
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage,
		void * inParam) {
}

int getFlapValue(int flap_ratio) {
	if (flap_ratio <= 100) {
		return flap_degree[flap_ratio];
	}
	return 4500;
}

/*
 * MyDrawingWindowCallback
 *
 * This callback does the work of drawing our window once per sim cycle each time
 * it is needed.  It dynamically changes the text depending on the saved mouse
 * status.  Note that we don't have to tell X-Plane to redraw us when our text
 * changes; we are redrawn by the sim continuously.
 *
 */
void MyDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon) {
	int l_nav1_standby_frequency_hz;
	float l_flap_ratio;
	int l_flap_ratio_int;
	int l_change;
	int l_comfreq;
	//double l_change_temp;
	int left, top, right, bottom;
	float color[] = { 1.0, 1.0, 1.0 }; /* RGB White */
	char cTmp[30];

	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	sprintf(cTmp, "USB/TWI : %d %d", g_shared_data.usbCounter, g_shared_data.twiCounter);
	XPLMDrawString(color, left - 20, bottom - 20, cTmp, NULL, xplmFont_Basic);

	if (deviceInitialized < 0) {
		countUSBInit++;
		if (countUSBInit > 1000) {
			writeLog("colomboard is not initialized. Trying to initialize.\n");
			closeDevice();
			deviceInitialized = initDevice();
			countUSBInit = 0;
		}
		return;
	}

	// check if boardvalues changed:
	if (g_shared_data.comStatusBoard == 1) {
		// update datarefs:
		g_comfreq_prev = g_shared_data.comFreq * 100 + g_shared_data.comFreqFraction;
		XPLMSetDatai(g_comfreq, g_comfreq_prev);
		g_shared_data.comStatusBoard = 0;

	}

	l_nav1_standby_frequency_hz = XPLMGetDatai(g_nav1_standby_frequency_hz);
	if (g_flap_ratio == NULL) {
		countDataRefInit++;
		if (countDataRefInit > 1000) {
			sprintf(cTmp, "Searching DataRef %s.\n", dataRefString);
			writeLog(cTmp);
			g_flap_ratio = XPLMFindDataRef(dataRefString);
			if (g_flap_ratio == NULL) {
				sprintf(cTmp, "Could not find DataRef %s.\n", dataRefString);
				writeLog(cTmp);
			} else {
				sprintf(cTmp, "DataRef %s found.\n", dataRefString);
				writeLog(cTmp);
			}
			countDataRefInit = 0;
		}
	} else {
		l_flap_ratio = XPLMGetDataf(g_flap_ratio);
	}
	//	sprintf(cTmp, "Nav1   : %3.2f",l_nav1_standby_frequency_hz / 1.0);
	//	XPLMDrawString(color, left - 20, bottom - 40, cTmp, NULL, xplmFont_Basic);
	sprintf(cTmp, "Flap   : %3.2f° %d", l_flap_ratio, getFlapValue(round(
			l_flap_ratio * 100.0)));
	XPLMDrawString(color, left - 20, bottom - 60, cTmp, NULL, xplmFont_Basic);

	l_comfreq = XPLMGetDatai(g_comfreq);

	sprintf(cTmp, "Radio   : %d", l_comfreq);
	XPLMDrawString(color, left - 20, bottom - 40, cTmp, NULL, xplmFont_Basic);


	if (l_flap_ratio != g_flap_ratio_prev) {
		g_shared_data.flapIndicator = getFlapValue(round(l_flap_ratio * 100.0));
		g_shared_data.comStatusHost = 1;
	}
	if (l_comfreq != g_comfreq_prev) {
		g_shared_data.comFreq = l_comfreq / 100;
		g_shared_data.comStatusHost = 1;
	}

	g_nav1_standby_frequency_hz_prev = l_nav1_standby_frequency_hz;
	g_comfreq_prev = l_comfreq;
}

/*
 * MyHandleKeyCallback
 *
 * Our key handling callback does nothing in this plugin.  This is ok;
 * we simply don't use keyboard input.
 *
 */
void MyHandleKeyCallback(XPLMWindowID inWindowID, char inKey,
		XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon,
		int losingFocus) {
}

/*
 * MyHandleMouseClickCallback
 *
 * Our mouse click callback toggles the status of our mouse variable
 * as the mouse is clicked.  We then update our text on the next sim
 * cycle.
 *
 */
int MyHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y,
		XPLMMouseStatus inMouse, void * inRefcon) {
	/* If we get a down or up, toggle our status click.  We will
	 * never get a down without an up if we accept the down. */
	if ((inMouse == xplm_MouseDown) || (inMouse == xplm_MouseUp))
		gClicked = 1 - gClicked;

	/* Returning 1 tells X-Plane that we 'accepted' the click; otherwise
	 * it would be passed to the next window behind us.  If we accept
	 * the click we get mouse moved and mouse up callbacks, if we don't
	 * we do not get any more callbacks.  It is worth noting that we
	 * will receive mouse moved and mouse up even if the mouse is dragged
	 * out of our window's box as long as the click started in our window's
	 * box. */
	//	return 1;
	return 0;
}
