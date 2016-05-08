/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
    HydraBus/HydraNFC - Copyright (C) 2014..2016 Benjamin VERNOUX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"

extern SerialUSBDriver SDU2;

#define VENDOR_ID	0x1d50
#define PRODUCT_ID	0x60a7

/*
 * Endpoints to be used for USBD2.
 */
#define USBD2_DATA_REQUEST_EP           1
#define USBD2_DATA_AVAILABLE_EP         1
#define USBD2_INTERRUPT_REQUEST_EP      2

/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
	USB_DESC_DEVICE(
		0x0110,        /* bcdUSB (1.1).                    */
		0x02,          /* bDeviceClass (CDC).              */
		0x00,          /* bDeviceSubClass.                 */
		0x00,          /* bDeviceProtocol.                 */
		0x40,          /* bMaxPacketSize.                  */
		VENDOR_ID,     /* idVendor.                        */
		PRODUCT_ID,    /* idProduct.                       */
		0x0200,        /* bcdDevice.                       */
		1,             /* iManufacturer.                   */
		2,             /* iProduct.                        */
		3,             /* iSerialNumber.                   */
		1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
	sizeof vcom_device_descriptor_data,
	vcom_device_descriptor_data
};

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_configuration_descriptor_data[67] = {
	/* Configuration Descriptor.*/
	USB_DESC_CONFIGURATION(
		67,            /* wTotalLength.                    */
		0x02,          /* bNumInterfaces.                  */
		0x01,          /* bConfigurationValue.             */
		0,             /* iConfiguration.                  */
		0xC0,          /* bmAttributes (self powered).     */
		50),           /* bMaxPower (100mA).               */
	/* Interface Descriptor.*/
	USB_DESC_INTERFACE(
		0x00,          /* bInterfaceNumber.                */
		0x00,          /* bAlternateSetting.               */
		0x01,          /* bNumEndpoints.                   */
		0x02,          /* bInterfaceClass (Communications
                                           Interface Class, CDC section
                                           4.2).                            */
		0x02,          /* bInterfaceSubClass (Abstract
                                         Control Model, CDC section 4.3).   */
		0x01,          /* bInterfaceProtocol (AT commands,
                                           CDC section 4.4).                */
		0),            /* iInterface.                      */
	/* Header Functional Descriptor (CDC section 5.2.3).*/
	USB_DESC_BYTE(5),            /* bLength.                         */
	USB_DESC_BYTE(0x24),         /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x00),         /* bDescriptorSubtype (Header
                                           Functional Descriptor.           */
	USB_DESC_BCD(0x0110),       /* bcdCDC.                          */
	/* Call Management Functional Descriptor. */
	USB_DESC_BYTE(5),            /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),         /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x01),         /* bDescriptorSubtype (Call Management
                                           Functional Descriptor).          */
	USB_DESC_BYTE(0x00),         /* bmCapabilities (D0+D1).          */
	USB_DESC_BYTE(0x01),         /* bDataInterface.                  */
	/* ACM Functional Descriptor.*/
	USB_DESC_BYTE(4),            /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),         /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x02),         /* bDescriptorSubtype (Abstract
                                           Control Management Descriptor).  */
	USB_DESC_BYTE(0x02),         /* bmCapabilities.                  */
	/* Union Functional Descriptor.*/
	USB_DESC_BYTE(5),            /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),         /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x06),         /* bDescriptorSubtype (Union
                                           Functional Descriptor).          */
	USB_DESC_BYTE(0x00),         /* bMasterInterface (Communication
                                           Class Interface).                */
	USB_DESC_BYTE(0x01),         /* bSlaveInterface0 (Data Class
                                           Interface).                      */
	/* Endpoint 2 Descriptor.*/
	USB_DESC_ENDPOINT(
		USBD2_INTERRUPT_REQUEST_EP|0x80,
		0x03,          /* bmAttributes (Interrupt). */
		0x0008,        /* wMaxPacketSize. */
		0xFF),         /* bInterval. */
	/* Interface Descriptor.*/
	USB_DESC_INTERFACE(
		0x01,          /* bInterfaceNumber. */
		0x00,          /* bAlternateSetting. */
		0x02,          /* bNumEndpoints. */
		0x0A,          /* bInterfaceClass (Data Class Interface, CDC section 4.5). */
		0x00,          /* bInterfaceSubClass (CDC section 4.6). */
		0x00,          /* bInterfaceProtocol (CDC section 4.7). */
		0x00),         /* iInterface. */
	/* Endpoint 3 Descriptor.*/
	USB_DESC_ENDPOINT(
		USBD2_DATA_AVAILABLE_EP,       /* bEndpointAddress.*/
		0x02,          /* bmAttributes (Bulk). */
		0x0040,        /* wMaxPacketSize. */
		0x00),         /* bInterval. */
	/* Endpoint 1 Descriptor.*/
	USB_DESC_ENDPOINT(
		USBD2_DATA_REQUEST_EP|0x80,    /* bEndpointAddress.*/
		0x02,          /* bmAttributes (Bulk). */
		0x0040,        /* wMaxPacketSize. */
		0x00)          /* bInterval. */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
	sizeof vcom_configuration_descriptor_data,
	vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[] = {
	USB_DESC_BYTE(4),                     /* bLength.                */
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.        */
	USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English). */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
	USB_DESC_BYTE(30),                    /* bLength.                         */
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
	'O', 0, 'p', 0, 'e', 0, 'n', 0, 'm', 0, 'o', 0, 'k', 0, 'o', 0,
	',', 0, ' ', 0, 'I', 0, 'n', 0, 'c', 0, '.', 0
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
	USB_DESC_BYTE(46),                    /* bLength. */
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType. */
	'H', 0, 'y', 0, 'd', 0, 'r', 0, 'a', 0, 'B', 0, 'u', 0, 's', 0,
	' ', 0, '1', 0, '.', 0, '0', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0,
	' ', 0, 'P', 0,  'o', 0, 'r', 0, 't', 0, '2', 0
};

/*
 * Serial Number string.
 */
static uint8_t vcom_string3[] = {
	USB_DESC_BYTE(50),                    /* bLength. */
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType. */
  /* Data filled by CPU with 96bits (12bytes) Serial Number from STM32 to ASCII HEX */
  ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00,
  ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00,
  ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00, ' ', 0x00
};

static const uint8_t htoa[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#define USB_DESCRIPTOR_SN_POS (2)
static void usb_descriptor_fill_string_serial_number(void)
{
  int i, j;
  uint32_t data_u32;
  uint8_t data_u8;
	uint32_t sn_32b[3];

	sn_32b[0] = *((uint32_t*)0x1FFF7A10);
	sn_32b[1] = *((uint32_t*)0x1FFF7A14);
	sn_32b[2] = *((uint32_t*)0x1FFF7A18);

  j = 0;
  for(i=0; i<3; i++)
  {
    data_u32 = sn_32b[i];

    data_u8 = (data_u32 & 0xFF000000) >> 24;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0xF0) >> 4];
    j+=2;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0x0F)];
    j+=2;

    data_u8 = (data_u32 & 0x00FF0000) >> 16;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0xF0) >> 4];
    j+=2;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0x0F)];
    j+=2;

    data_u8 = (data_u32 & 0x0000FF00) >> 8;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0xF0) >> 4];
    j+=2;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0x0F)];
    j+=2;

    data_u8 = (data_u32 & 0x000000FF);
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0xF0) >> 4];
    j+=2;
    vcom_string3[USB_DESCRIPTOR_SN_POS + j] = htoa[(data_u8 & 0x0F)];
    j+=2;
  }
}

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {
	{sizeof vcom_string0, vcom_string0},
	{sizeof vcom_string1, vcom_string1},
	{sizeof vcom_string2, vcom_string2},
	{sizeof vcom_string3, vcom_string3}
};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
		uint8_t dtype,
		uint8_t dindex,
		uint16_t lang)
{

	(void)usbp;
	(void)lang;
	switch (dtype) {
	case USB_DESCRIPTOR_DEVICE:
		return &vcom_device_descriptor;
	case USB_DESCRIPTOR_CONFIGURATION:
		return &vcom_configuration_descriptor;
	case USB_DESCRIPTOR_STRING:
		if (dindex < 4)
		{
			if(dindex == 3)
					usb_descriptor_fill_string_serial_number();
			return &vcom_strings[dindex];
		}
	}
	return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
	USB_EP_MODE_TYPE_BULK,
	NULL,
	sduDataTransmitted,
	sduDataReceived,
	0x0040,
	0x0040,
	&ep1instate,
	&ep1outstate,
	2,
	NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
	USB_EP_MODE_TYPE_INTR,
	NULL,
	sduInterruptTransmitted,
	NULL,
	0x0010,
	0x0000,
	&ep2instate,
	NULL,
	1,
	NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event)
{
	switch (event) {
	case USB_EVENT_RESET:
		return;
	case USB_EVENT_ADDRESS:
		return;
	case USB_EVENT_CONFIGURED:
		chSysLockFromISR();

		/* Enables the endpoints specified into the configuration.
		   Note, this callback is invoked from an ISR so I-Class functions
		   must be used.*/
		usbInitEndpointI(usbp, USBD2_DATA_REQUEST_EP, &ep1config);
		usbInitEndpointI(usbp, USBD2_INTERRUPT_REQUEST_EP, &ep2config);

		/* Resetting the state of the CDC subsystem.*/
		sduConfigureHookI(&SDU2);

		chSysUnlockFromISR();
		return;
	case USB_EVENT_SUSPEND:
		chSysLockFromISR();

		/* Disconnection event on suspend.*/
		sduDisconnectI(&SDU2);

		chSysUnlockFromISR();
		return;
	case USB_EVENT_WAKEUP:
		return;
	case USB_EVENT_STALLED:
		return;
	}
	return;
}

/*
 * Handles the USB driver global events.
 */
static void sof_handler(USBDriver *usbp) {

  (void)usbp;

  osalSysLockFromISR();
  sduSOFHookI(&SDU2);
  osalSysUnlockFromISR();
}

/*
 * USB driver configuration.
 */
const USBConfig usb2cfg = {
	usb_event,
	get_descriptor,
	sduRequestsHook,
	sof_handler
};

/*
 * Serial over USB driver configuration.
 */
const SerialUSBConfig serusb2cfg = {
	&USBD2,
	USBD2_DATA_REQUEST_EP,
	USBD2_DATA_AVAILABLE_EP,
	USBD2_INTERRUPT_REQUEST_EP
};
