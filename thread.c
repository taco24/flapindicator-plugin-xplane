#if IBM
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "thread.h"
#include "colomboard.h"
#include "time.h"
#include "log.h"

#define MAX_LINE 256
int g_comFreq;
int g_comFreqFraction;
int g_comFreqStandby;
int g_comFreqStandbyFraction;
int g_navFreq;
int g_navFreqFraction;
int g_navFreqStandby;
int g_navFreqStandbyFraction;
int g_flapIndicator;
int g_usbCounter;
int g_twiCounter;
int g_counter;
struct usb_data g_usb_data;
static const int min_mainloop_time = 5000;
static long last_mainloop_idle = 0;
struct shared_data *g_ptr_shared_data;

void updateHost() {
	g_ptr_shared_data->usbCounter = g_usb_data.usbCounter;
	g_ptr_shared_data->twiCounter = g_usb_data.twiCounter;
	if (g_ptr_shared_data->comStatusBoard == 1) {
		// wait until change has been processed
	} else {
		// update host
		if (g_usb_data.comStatusBoard == 1) {
			// Board changed:
			g_ptr_shared_data->comFreq = g_usb_data.comFreqIn;
			g_ptr_shared_data->comStatusBoard = 1;
			g_comFreq = g_usb_data.comFreqIn;
			g_usb_data.comStatusBoard = 0;
			//	} else if (g_comFreq != g_usb_data.comFreqIn) {
			//		g_ptr_shared_data->comFreq = g_usb_data.comFreqIn;
			//		g_comFreq = g_usb_data.comFreqIn;
		}
		if (g_comFreqFraction != g_ptr_shared_data->comFreqFraction) {
			g_comFreqFraction = g_ptr_shared_data->comFreqFraction;
		} else if (g_comFreqFraction != g_usb_data.comFreqFraction) {
			g_comFreqFraction = g_usb_data.comFreqFraction;
		}
		if (g_comFreqStandby != g_ptr_shared_data->comFreqStandby) {
			g_comFreqStandby = g_ptr_shared_data->comFreqStandby;
		} else if (g_comFreqStandby != g_usb_data.comFreqStandby) {
			g_comFreqStandby = g_usb_data.comFreqStandby;
		}
		if (g_comFreqStandbyFraction
				!= g_ptr_shared_data->comFreqStandbyFraction) {
			g_comFreqStandbyFraction
					= g_ptr_shared_data->comFreqStandbyFraction;
		} else if (g_comFreqStandbyFraction
				!= g_usb_data.comFreqStandbyFraction) {
			g_comFreqStandbyFraction = g_usb_data.comFreqStandbyFraction;
		}
	}
	if (g_flapIndicator != g_ptr_shared_data->flapIndicator) {
		g_flapIndicator = g_ptr_shared_data->flapIndicator;
	} else if (g_flapIndicator != g_usb_data.flapIndicator) {
		g_flapIndicator = g_usb_data.flapIndicator;
	}

}

void updateBoard() {
	char cTmp[MAX_LINE];
	static int toggle = 0;
	switch (toggle) {
	case 0:
		if (g_ptr_shared_data->comStatusBoard == 1) {
			// wait until change has been processed
		} else {
			g_usb_data.comStatusHost = g_ptr_shared_data->comStatusHost;
			g_ptr_shared_data->comStatusHost = 0;
			g_usb_data.comFreq = g_ptr_shared_data->comFreq;
			g_usb_data.comFreqFraction = g_comFreqFraction;
			g_usb_data.comFreqStandby = g_comFreqStandby;
			g_usb_data.comFreqStandbyFraction = g_comFreqStandbyFraction;
			g_usb_data.flapIndicator = g_flapIndicator;
			writeDeviceCom(&g_usb_data);
		}
		toggle++;
		break;
	case 1:
		writeDeviceNav(&g_usb_data);
		toggle++;
		break;
	default:
		toggle = 0;
	}
}

void *run(void *ptr_shared_data) {
	char cTmp[MAX_LINE];

	int l_change = 0;

	g_ptr_shared_data = (struct shared_data *) ptr_shared_data;
	// Initialize datarefs
	g_comFreq = g_ptr_shared_data->comFreq;

	last_mainloop_idle = sys_time_clock_get_time_usec();
	// while stop == 0 calculate position.
	while (g_ptr_shared_data->stop == 0) {
		long loop_start_time = sys_time_clock_get_time_usec();

		///////////////////////////////////////////////////////////////////////////
		/// CRITICAL FAST 20 Hz functions
		///////////////////////////////////////////////////////////////////////////
		if (us_run_every(50000, COUNTER3, loop_start_time)) {
			// read usb board values
			l_change = readDevice(&g_usb_data);
			// Update xplane
			updateHost();
			// Update board
			updateBoard();
		}
		///////////////////////////////////////////////////////////////////////////

		///////////////////////////////////////////////////////////////////////////
		/// NON-CRITICAL SLOW 10 Hz functions
		///////////////////////////////////////////////////////////////////////////
		else if (us_run_every(100000, COUNTER6, loop_start_time)) {
			//update_screen();
		}
		///////////////////////////////////////////////////////////////////////////

		if (loop_start_time - last_mainloop_idle >= 100000) {
			writeLog("CRITICAL WARNING! CPU LOAD TOO HIGH.\n");
			last_mainloop_idle = loop_start_time;//reset to prevent multiple messages
		} else {
			//writeConsole(0, 0, "CPU LOAD OK.");
		}

		// wait 1 milliseconds
#if IBM
		Sleep(1);
#endif
#if LIN
		usleep(10);
#endif
		g_counter++;
	}
	sprintf(cTmp, "thread closing usb device %d...\n", 0);
	writeLog(cTmp);
	closeDevice();
	sprintf(cTmp, "thread info : stopping thread #%d, %d!\n",
			g_ptr_shared_data->thread_id, g_counter);
	writeLog(cTmp);
	pthread_exit(NULL);
	return 0;
}

