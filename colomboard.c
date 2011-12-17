/*
 * colomboard.c
 *
 */
#include <usb.h>
#include <stdio.h>
#include "thread.h"
#include "colomboard.h"
#include "log.h"

#define VERSION "0.1.0"
#define MY_VID 0xF055
#define MY_PID 0x5500
//#define MY_VID 0x03EB
//#define MY_PID 0x20CC
/* the device's endpoints */
#define EP_IN 0x81
#define EP_OUT 0x02

#define BUF_SIZE 8
#define buf_size 8
#define MAX_LINE 256

usb_dev_handle *dev;
int writeCounter = 0;
char tmp[BUF_SIZE];
char tmp2[BUF_SIZE];

int usb_status = 0;
int returnCode = 0;

int initDevice();
int closeDevice();
int readDevice();
int writeDevice(void *ptr_usb_data);

void fatalError(const char *why) {
	fprintf(stderr, "Fatal error> %s\n", why);
	//	exit(17);
}

usb_dev_handle *find_colomboard_libusb();

usb_dev_handle* setup_libusb_access() {
#ifdef __linux__
	char cTmp[255];
#endif
	usb_dev_handle *colomboard_libusb;
	//	usb_set_debug(255);
	usb_set_debug(0);
	usb_init();
	usb_find_busses();
	usb_find_devices();

	if (!(colomboard_libusb = find_colomboard_libusb())) {
		printf("Couldn't find the USB device, Exiting\n");
		return NULL;
	}
#ifdef __linux__
	// linux detach kernel driver
	if ((returnCode = usb_detach_kernel_driver_np(colomboard_libusb, 0)) < 0) {
		sprintf(cTmp, "Could not detach kernel driver: %d", returnCode);
		writeLog(cTmp);
	}
#endif
	if ((returnCode = usb_set_configuration(colomboard_libusb, 1)) < 0) {
		printf("Could not set configuration 1 : %d\n", returnCode);
		return NULL;
	}
	if ((returnCode = usb_claim_interface(colomboard_libusb, 0)) < 0) {
		printf("Could not claim interface: %d\n", returnCode);
		return NULL;
	}
	return colomboard_libusb;
}

usb_dev_handle *find_colomboard_libusb() {
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == MY_VID && dev->descriptor.idProduct
					== MY_PID) {
				return usb_open(dev);
			}
		}
	}
	return NULL;
}

int writeDeviceCom(void *ptr_usb_data) {
	if (usb_status < 0) {
		return usb_status;
	}
	struct usb_data *l_ptr_usb_data;
	l_ptr_usb_data = (struct usb_data *) ptr_usb_data;
	tmp[0] = 10;
	tmp[1] = l_ptr_usb_data->comStatusHost;
	tmp[2] = l_ptr_usb_data->comFreq;
	tmp[3] = l_ptr_usb_data->comFreqFraction;
	tmp[4] = l_ptr_usb_data->comFreqStandby;
	tmp[5] = l_ptr_usb_data->comFreqStandbyFraction;
	tmp[6] = l_ptr_usb_data->flapIndicator / 255;
	tmp[7] = l_ptr_usb_data->flapIndicator % 255;
	if ((usb_status = usb_interrupt_write(dev, EP_OUT, tmp, sizeof(tmp), 5000))
			!= sizeof(tmp)) {
		printf("error: interrupt write failed\n");
	}
	return 0;

}

int writeDeviceNav(void *ptr_usb_data) {
	if (usb_status < 0) {
		return usb_status;
	}
	struct usb_data *l_ptr_usb_data;
	l_ptr_usb_data = (struct usb_data *) ptr_usb_data;
	tmp[0] = 20;
	tmp[1] = l_ptr_usb_data->navStatusHost;
	tmp[2] = l_ptr_usb_data->navFreq;
	tmp[3] = l_ptr_usb_data->navFreqFraction;
	tmp[4] = l_ptr_usb_data->navFreqStandby;
	tmp[5] = l_ptr_usb_data->navFreqStandbyFraction;
	if ((usb_status = usb_interrupt_write(dev, EP_OUT, tmp, sizeof(tmp), 5000))
			!= sizeof(tmp)) {
		printf("error: interrupt write failed\n");
	}
	return 0;

}

int writeDevice(void *ptr_usb_data) {
	if (usb_status < 0) {
		return usb_status;
	}
	struct usb_data *l_ptr_usb_data;
	l_ptr_usb_data = (struct usb_data *) ptr_usb_data;
	// write colomboard
	tmp[0] = 10;
	tmp[1] = l_ptr_usb_data->comStatusHost;
	tmp[2] = l_ptr_usb_data->comFreq;
	tmp[3] = l_ptr_usb_data->comFreqFraction;
	tmp[4] = l_ptr_usb_data->comFreqStandby;
	tmp[5] = l_ptr_usb_data->comFreqStandbyFraction;
	tmp[6] = l_ptr_usb_data->flapIndicator / 255;
	tmp[7] = l_ptr_usb_data->flapIndicator % 255;
	if ((usb_status = usb_interrupt_write(dev, EP_OUT, tmp, sizeof(tmp), 5000))
			!= sizeof(tmp)) {
		printf("error: interrupt write failed\n");
	}
	return 0;
}

int getValue(unsigned char a, unsigned char b) {
	int result = ((int) a) * 255 + ((int) b);
	return result;
}

int readDevice(void *ptr_usb_data) {
	int countBytes = 0;
	if (usb_status < 0) {
		return usb_status;
	}
	struct usb_data *l_ptr_usb_data;
	l_ptr_usb_data = (struct usb_data *) ptr_usb_data;
	// read colomboard
	signed char result = 0;
	countBytes = usb_interrupt_read(dev, EP_IN, tmp2, sizeof(tmp2), 5000);
	if (countBytes != sizeof(tmp2)) {
		printf("error: interrupt read failed\n");
		printf("%d\n", countBytes);
		result = -1;
	} else {
		if (tmp2[0] == 10) {
			l_ptr_usb_data->comStatusBoard = (unsigned char) tmp2[1];
			l_ptr_usb_data->comFreqIn = (unsigned char) tmp2[2];
			l_ptr_usb_data->comFreqFractionIn = (unsigned char) tmp2[3];
			l_ptr_usb_data->comFreqStandbyIn = (unsigned char) tmp2[4];
			l_ptr_usb_data->comFreqStandbyFractionIn = (unsigned char) tmp2[5];
			l_ptr_usb_data->flapIndicator = getValue(tmp2[6], tmp2[7]);
		} else if (tmp2[0] == 20) {
			l_ptr_usb_data->navStatusBoard = (unsigned char) tmp2[1];
			l_ptr_usb_data->navFreq = (unsigned char) tmp2[2];
			l_ptr_usb_data->navFreqFraction = (unsigned char) tmp2[3];
			l_ptr_usb_data->navFreqStandby = (unsigned char) tmp2[4];
			l_ptr_usb_data->navFreqStandbyFraction = (unsigned char) tmp2[5];
			l_ptr_usb_data->usbCounter = (unsigned char) tmp2[6];
			l_ptr_usb_data->twiCounter = (unsigned char) tmp2[7];
		} else {
			l_ptr_usb_data->board0 = tmp2[0];
			l_ptr_usb_data->board1 = tmp2[1];
			l_ptr_usb_data->board2 = tmp2[2];
			l_ptr_usb_data->board3 = tmp2[3];
			l_ptr_usb_data->board4 = tmp2[4];
			l_ptr_usb_data->board5 = tmp2[5];
			l_ptr_usb_data->board6 = tmp2[6];
			l_ptr_usb_data->board7 = tmp2[7];
		}
		return result;
	}
}

char* getVersionDevice() {
	return VERSION;
}

int initDevice() {
	if ((dev = setup_libusb_access()) == NULL) {
		usb_status = -1;
		return -1;
	}
	usb_status = 0;
	return 0;
}

int closeDevice() {
	if (dev == NULL) {
		usb_status = -1;
		return -1;
	} else {
		usb_release_interface(dev, 0);
		usb_close(dev);
	}
	usb_status = 0;
	return 0;
}
