/* libD4000.cpp
Formerly D4000_usb.cpp and DDC4000Ctl.cpp
Original copyright by Texas Instruments
Ported to Linux (and other *nix) via libusb0.1 with permission from TI 
by Dr. Daniel Levner, Church Lab, Harvard Medical School
Copyright Daniel Levner & Texas Instruments, Inc. (c) 2009
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <usb.h>

#include "porting.h"
#include "libD4000.h"


//Original Driver
#define MAX_TRANSFER_SIZE 63488 //In bytes, make a multiple of 512
#define DLLVERSION 0x00010000 //Top 16 bits is Major version, bottom 16 bits is minor version


#define USB_TIMEOUT 1000 	//default timeout for USB requests in ms
#define DEVICEHANDLE_NUM 15	//change this number to change maximum number of devices connected at once



usb_dev_handle *DeviceHandle[DEVICEHANDLE_NUM];			// initialized by libD4000_init()
unsigned short DeviceInterface[DEVICEHANDLE_NUM];		// claimed interface for each DeviceHandle
struct usb_device *DiscoveredDevices[DEVICEHANDLE_NUM];		// initialized by libD4000_init()
struct usb_device DiscoveredDevices_records[DEVICEHANDLE_NUM];
int last_DiscoveredDevice = 0;

//Private DLL functions

/* Scans the USB busses to find all DMD devices. 
   Maintains a numbered list of these, to correspond to ezusb-n under windows.
   Devices are deemed the same if they share filenames
*/
void discover_devices(void)
{
	struct usb_bus *bus, *bus_head;
	struct usb_device *dev, *new_devices[DEVICEHANDLE_NUM];
	int idx, idx2, num_new_devices=0;
	BOOL found[DEVICEHANDLE_NUM], this_found;


	// initialize found[]
	for(idx = 0; idx < DEVICEHANDLE_NUM; idx++)
		found[idx] = 0;


	// scan the USB bus
	usb_find_busses();
	bus_head = usb_get_busses();
	if(!usb_find_devices()) return;	// can stop now if no changes in device list since last run


	for (bus = bus_head; bus; bus = bus->next)
    		for (dev = bus->devices; dev; dev = dev->next)
		{
    			// check if this is a D4000 DMD
    			if ((dev->descriptor.idVendor == DMD_USB_idVendor) && (dev->descriptor.idProduct == DMD_USB_idProduct))
			{
				// check if it's in DiscoveredDevices
				this_found = 0;
				for(idx = 0; idx < DEVICEHANDLE_NUM; idx++)
				{
					if((dev != NULL) && (strcmp(dev->filename, DiscoveredDevices_records[idx].filename) == 0))
					{
						// mark that we've found this device in our records
						this_found = 1;
						found[idx] = 1;

						// update the record in case anything else changed (esp. dev pointer)
						DiscoveredDevices[idx] = dev;
						DiscoveredDevices_records[idx] = *dev;
						break;	// stop going through DiscoveredDevices
					}
				}

				if(!this_found)	// case if found already taken care of
				{
					if(num_new_devices < DEVICEHANDLE_NUM)	// avoid overflows
					{
						// keep track of the new device
						new_devices[num_new_devices] = dev;
						num_new_devices++;
					}
				}

    			} // if a DMD; don't need to do anything if not

		} // fordevices, for busses

	// get rid of all the entries that have not been found
	for(idx = 0; idx < DEVICEHANDLE_NUM; idx++)
		if(found[idx] == 0)	// if not found
			DiscoveredDevices[idx] = NULL;

	// fill in the newly found devices
	for(idx = 0; idx < num_new_devices; idx++)
	{
		// find an empty spot
		this_found = 0;
		for(idx2 = 0; idx2 < DEVICEHANDLE_NUM; idx2++)
			if(DiscoveredDevices[idx2] == NULL)
			{
				this_found = 1;
				break;
			}

		// quit if there are no new spots
		if(!this_found)
			break;

		// put the new record in the found empty spot
		DiscoveredDevices[idx2] = new_devices[idx];
		DiscoveredDevices_records[idx2] = *new_devices[idx];

		// see if we have to increment last_DiscoveredDevice
		if(idx2 > last_DiscoveredDevice)
			last_DiscoveredDevice = idx2;
	}
}



void VendRequestOut(UCHAR Request, short int DeviceNumber)
{
	VENDOR_OR_CLASS_REQUEST_CONTROL	myRequest;
	UCHAR buf[2];

	myRequest.request = Request; //Request for register read/write
	myRequest.index = 0x0000;
	myRequest.value = 0x0000;
	myRequest.direction = 0; //0 OUT, 1 IN
	myRequest.requestType = USB_RECIP_DEVICE | USB_TYPE_VENDOR | USB_ENDPOINT_OUT; //Vendor Request
	myRequest.recepient = 0; //To Device

	usb_control_msg(DeviceHandle[DeviceNumber], // handle to device of interest
		myRequest.requestType,
		myRequest.request,
		myRequest.value,
		myRequest.index,
		(char *)buf, // pointer to buffer to receive output data
		2, // size of output buffer
		USB_TIMEOUT
		);
}


BOOL VendRequestIn(UCHAR Request, UCHAR buf[2], short int DeviceNumber)
{
	int result = 0;
	VENDOR_OR_CLASS_REQUEST_CONTROL	myRequest;

	myRequest.request = Request; //Request for register read/write
	myRequest.index = 0x0000;
	myRequest.value = 0x0000;
	myRequest.direction = 1; //0 OUT, 1 IN
	myRequest.requestType = USB_RECIP_DEVICE | USB_TYPE_VENDOR | USB_ENDPOINT_IN; //Vendor Request
	myRequest.recepient = 0; //To Device

	result = usb_control_msg(DeviceHandle[DeviceNumber], // handle to device of interest
		myRequest.requestType,
		myRequest.request,
		myRequest.value,
		myRequest.index,
		(char *)buf, // pointer to buffer to receive output data
		2, // size of output buffer
		USB_TIMEOUT
		);

	return (BOOL)(result>=0);
}



//Swap B1B2 to B2B1
void BreakSwapBytes(UCHAR file_buffer_swapped[][MAX_TRANSFER_SIZE], UCHAR *file_buffer, LONG *file_size, int NumberOfTransfers)
{
	int BitField[256];
	long fsize = *file_size;
	int counter = 0;
	int i = 0;
	long q = 0;

	for(i = 0; i < 256;i++) //Create the Lookup table for swaps
	{
		int k = i;
		BitField[i] = 0;
		for(int b=0;b<8;b++)
		{
			BitField[i]<<=1;
			BitField[i] |= k & 1;
			k >>= 1;
		}
	}
	
	for(i=0; i < NumberOfTransfers && counter < fsize; i++) //Swap the old data and put into buffer
	{
		for(q=0; q < MAX_TRANSFER_SIZE && counter < fsize; q++)
		{
			file_buffer_swapped[i][q] = BitField[file_buffer[counter]];
			counter++;
		}
	}
	for(; q < MAX_TRANSFER_SIZE;q++) //Must zero out the last transfer to make it a nice even transfer (multiple of 512)
		file_buffer_swapped[i-1][q] = 0x00;
}



/* Undo what open_handle() does */
void close_handle(short int DeviceNumber)
{
	if(DeviceHandle[DeviceNumber] != NULL)
	{
		usb_release_interface(DeviceHandle[DeviceNumber], DeviceInterface[DeviceNumber]);
		usb_close(DeviceHandle[DeviceNumber]);
		DeviceHandle[DeviceNumber] = NULL;
	}
}



//Connect to the device with the specified handle
/* if DeviceNumber is low enough that we should have seen it (based on last_DiscoveredDevice)
   then try to open the device in our list.  If not, try to discover more devices.  However, if
   we fail in opening a known device, run a discovery and try again. */
int open_handle(short int DeviceNumber)
{
	int ran_discovery = 0;
	int my_interface, retval;
	struct usb_device *my_dev;

	// see if DeviceNumber corresponds to a previously discovered device
	if(DiscoveredDevices[DeviceNumber] == NULL)
	{
		// if no known device, run a discovery
		discover_devices();
		ran_discovery = 1;

		// if it's still not discovered, give up
		if(DiscoveredDevices[DeviceNumber] == NULL)
			return -1;
	}

	// try to open the device 
	DeviceHandle[DeviceNumber] = usb_open(DiscoveredDevices[DeviceNumber]);

	// see if this worked
	if(DeviceHandle[DeviceNumber] == NULL)
	{
		// didn't work; discover devices if we haven't already and try again
		if(!ran_discovery)
		{
			discover_devices();

			// if we found a device, try to open it
			if(DiscoveredDevices[DeviceNumber] != NULL)
				DeviceHandle[DeviceNumber] = usb_open(DiscoveredDevices[DeviceNumber]);
			else
				return -1;

			// check again if usb_open() worked
			if(DeviceHandle[DeviceNumber] == NULL)
				return -1; //Means the open device operation failed
		}
		else
			return -1;	// discovery won't help; this didn't work

	}

	// we should only be here if usb_open() was successful
	// must claim an interface!! (according to libusb manual)

	// find first interface or set interface = 0
	my_dev = DiscoveredDevices[DeviceNumber];
	if((my_dev->config != NULL) && (my_dev->config->interface !=NULL) && (my_dev->config->interface->altsetting !=NULL)) 
		my_interface = my_dev->config->interface->altsetting->bInterfaceNumber;
	else
		my_interface = 0;
	
	// keep track of the interface that we want to claim
	DeviceInterface[DeviceNumber] = my_interface;

	// claim interface and check for error
	retval = usb_claim_interface(DeviceHandle[DeviceNumber], my_interface);
	if(retval < 0)
	{
		// could not get the interface -- abort
		close_handle(DeviceNumber);
		return -2;	// new error value == claim_interface failed
	}
}



//Will write the value in Data to the location Register on Device DeviceNumber
short int Register_Write(unsigned short int Register,unsigned short int Data, short int DeviceNumber)
{
	VENDOR_OR_CLASS_REQUEST_CONTROL	myRequest;
	UCHAR buf[2];
	int result;
	
	if(open_handle(DeviceNumber) < 0)
		return 0; //USB device failed to open

	myRequest.request = 0xBA; //Request for register read/write
	myRequest.value = Register; //Address to send to
	myRequest.index = 0x0000;
	myRequest.direction = 0; //0 OUT, 1 IN
	myRequest.requestType = USB_RECIP_DEVICE | USB_TYPE_VENDOR | USB_ENDPOINT_OUT; //Vendor Request
	myRequest.recepient = 0; //recepient is device (0)
		
	for (int i = 1; i >= 0; i--, Data/= 256) //Pull least significant 2 bytes out of the integer
		buf[i] = Data % 256;

	result = usb_control_msg(DeviceHandle[DeviceNumber], // handle to device of interest
		myRequest.requestType,
		myRequest.request,
		myRequest.value,
		myRequest.index,
		(char *)buf, // pointer to buffer to receive output data
		2, // size of output buffer
		USB_TIMEOUT
		);

	if(result < 0)
	{
		close_handle(DeviceNumber);
		return 0; //Register_Write request failed
	}

	close_handle(DeviceNumber);
	return 1;
}



//Returns the value at location Register on Device DeviceNumber
unsigned short int Register_Read(unsigned short int Register, short int DeviceNumber)
{
	short int output = 0;

	VENDOR_OR_CLASS_REQUEST_CONTROL	myRequest;
	UCHAR buf[2];
	int result, i;
		
	if(open_handle(DeviceNumber) < 0)
		return -1; //USB device failed to open

	myRequest.request = 0xBA; //Request for register read/write
	myRequest.index = 0x0000;
	myRequest.value = Register; //Register address
	myRequest.direction = 1; //0 OUT, 1 IN
	myRequest.requestType = USB_RECIP_DEVICE | USB_TYPE_VENDOR | USB_ENDPOINT_IN; //Vendor Request
	myRequest.recepient = 0; //Device

	result = usb_control_msg(DeviceHandle[DeviceNumber], // handle to device of interest
		myRequest.requestType,
		myRequest.request,
		myRequest.value,
		myRequest.index,
		(char *)buf, // pointer to buffer to receive output data
		2, // size of output buffer
		USB_TIMEOUT
		);

	if(result < 0)
	{
		close_handle(DeviceNumber);
		return -2; //Register_Read request failed
	}
		
	for(i = 0; i < 2; i++) //Put the Uchar array into an integer
		output = ( output << 8 ) | buf[i];
		
	close_handle(DeviceNumber);
	return output;
}



//Transfered packets must be multiples of 512, so zero pad the end if it is not
int Data_Write(UCHAR *write_buffer, long write_size, short int DeviceNumber)
{
	UCHAR *LastTransfer;
	long padded_length=0;
	long first_length = 0;
	short padding_length=0;
	int result, i;

	if(open_handle(DeviceNumber) < 0)
		return 0; //USB device failed to open
		

	if(write_size % 512 != 0)	//If the transfer is not an even FIFO size it must be zero padded
	{
		padding_length = 512 - (write_size % 512);
		padded_length = write_size + padding_length;
		first_length = write_size - (write_size % 512); 
		
		LastTransfer = new UCHAR[512 + 1];
		for(i = 0;i< (512 - padding_length);i++)
			LastTransfer[i] = write_buffer[write_size - (512-padding_length) + i]; //Copy the actual data into the beginning of the last transfer buffer
		for(i = (512-padding_length);i<512;i++) //Fill the rest with 0's
			LastTransfer[i] = 0x00;
	}
	else
		first_length = write_size;


	result = usb_bulk_write(DeviceHandle[DeviceNumber], 	// handle to device of interest
		DMD_USB_EP_BULK_OUT | USB_ENDPOINT_OUT,		// Endpoint 2, bulk OUT transfer
		(char *)write_buffer, 				// pointer to buffer for output data
		first_length, 					// size of output buffer
		USB_TIMEOUT
		);

	if(write_size % 512 != 0) // Only do a second transfer to make sure the total transfer length is a multiple of 512
	{
		result = usb_bulk_write(DeviceHandle[DeviceNumber], 	// handle to device of interest
			DMD_USB_EP_BULK_OUT | USB_ENDPOINT_OUT,		// Endpoint 2, bulk OUT transfer
			(char *)LastTransfer,				// pointer to buffer for output data
			512,						// size of output buffer
			USB_TIMEOUT
			);
	}

	if(result <= 0)
	{
		delete LastTransfer;
		close_handle(DeviceNumber);
		return 0; //Data_Write request failed
	}
	
	delete LastTransfer;
	close_handle(DeviceNumber);
	return 1; //Return true
}



//Must read a multiple of 512
int Data_Read(UCHAR *read_buffer, unsigned int read_size, short int DeviceNumber)
{
	int result;

	if(open_handle(DeviceNumber) < 0)
		return -1; //USB device failed to open

	VendRequestOut(0xB3,DeviceNumber); //Send the device an 0xB3 to signify read request

	result = usb_bulk_read(DeviceHandle[DeviceNumber], 	// handle to device of interest
		DMD_USB_EP_BULK_IN | USB_ENDPOINT_IN,		// Endpoint 86, bulk IN transfer
		(char *)read_buffer, 				// pointer to buffer to receive data
		read_size, 					// size of buffer
		USB_TIMEOUT
		);

	if(result < 0)
	{
		close_handle(DeviceNumber);
		return -2; //Data_Read request failed
	}

	close_handle(DeviceNumber);
	return result;	//Return the number of bytes transfered
}




/*
// Public functions
*/

extern "C"
{
	int libD4000_init()
	{
		int idx;

		usb_init();		// initialize libusb

		// initialize DeviceHandle and DiscoveredDevices arrays
		for(idx = 0; idx < DEVICEHANDLE_NUM; idx++)
			DeviceHandle[idx] = (usb_dev_handle *)NULL;
		for(idx = 0; idx < DEVICEHANDLE_NUM; idx++)
			DiscoveredDevices[idx] = (struct usb_device *)NULL;
		last_DiscoveredDevice = 0;

		discover_devices();	// tabulate the DMDs currently attached
		
		return 0;
	}


	/*
	Returns the number of connected devices
	Closes the handles at the end to keep the USB device from freezing if one is unplugged then plugged back in
	Each funtion will re-open the handles, and then close them when it is done
	*/
	short int libD4000_GetNumDev()
	{
		int count = 0;
		short i = 0;

		for(i = 0; i < DEVICEHANDLE_NUM; i++)//Create as many handles as there are devices connected
		{
			if(open_handle(i) >=0)
				count++;
		}

		for(i = 0; i < DEVICEHANDLE_NUM; i++) //Must close the handles after each operation to avoid hard resets
			close_handle(i);

		return count; //Return the actual number of devices
	}


	// Pass array of ints for descriptor values
	// Return the number of bytes actually transfered
	int libD4000_GetDescriptor(int* Array, short int DeviceNumber)
	{
		struct usb_device *dev;
		
		// the descriptor is readily available when using libusb 

		discover_devices();	// capture all DMDs plugged in

		// check if the device being asked for is there
		dev = DiscoveredDevices[DeviceNumber];
		if(dev == NULL)
			return -1;	// device not found



		//Assign a value in the array for each descriptor value
		Array[0] = dev->descriptor.bLength;
		Array[1] = dev->descriptor.bDescriptorType;
		Array[2] = dev->descriptor.bcdUSB;
		Array[3] = dev->descriptor.bDeviceClass;
		Array[4] = dev->descriptor.bDeviceSubClass;
		Array[5] = dev->descriptor.bDeviceProtocol;
		Array[6] = dev->descriptor.bMaxPacketSize0;
		Array[7] = dev->descriptor.idVendor;
		Array[8] = dev->descriptor.idProduct;
		Array[9] = dev->descriptor.bcdDevice;
		Array[10] = dev->descriptor.iManufacturer;
		Array[11] = dev->descriptor.iProduct;
		Array[12] = dev->descriptor.iSerialNumber;
		Array[13] = dev->descriptor.bNumConfigurations;

		return 0x12;	// fake -- no. of bytes returned by descriptor request
	}


	//Send a vendor request to get the firmware version number
	short libD4000_GetFirmwareRev(short int DeviceNumber)
	{
		UCHAR buf[2];
		DWORD nBytes = 0;
		short int output = 0;
		BOOL result;

		if(open_handle(DeviceNumber) < 0)
			return -1; //USB device failed to open

		result = VendRequestIn(0xBD, buf, DeviceNumber);
		if(result == 0)
		{
			close_handle(DeviceNumber);
			return -2; //Vendor request failed
		}

		//Shift both bytes from the UCHAR buf into an int
		output = buf[0];
		output = output << 8;
		output = output & 0xFF00; //Clear the bottom 4 bits
		output += buf[1];

		close_handle(DeviceNumber);
		return output;
	}


	
	unsigned int libD4000_GetDLLRev()
	{
		return DLLVERSION;
	}



	short libD4000_GetRESETCOMPLETE(int waittime, short int DeviceNumber)
	{
		//looptime = time in milliseconds to loop..  We will loop every 100 milliseconds, if loop time = 0 loop forever
		int num_of_loops = waittime/100;
		short reset_complete = 0; 
		
		//enable extrnal resets, clearing it and setting it clears the reset complete flag on the D4000
		libD4000_SetEXTRESETENBL(1,DeviceNumber);
		
		if (num_of_loops == 0) 
		{
			while(1)
			{
				if(Register_Read(D4000_RESET_COMPLETE,DeviceNumber) == 1)
					reset_complete = 1;
				    break;
			}
		}
		else
		{

			//wait 10 seconds for reset to happen. Return 0 if reset complete doesn't happen in 10 seconds.
			for(int i = 0;i < num_of_loops; i++)
			{
				if(Register_Read(D4000_RESET_COMPLETE,DeviceNumber) == 1)
				{	
					reset_complete = 1;
					i = 1000;
				}
				else
					Sleep(100);
			}
		}

		if (reset_complete == 0)
			return 0;

		//send two no-ops

		libD4000_SetBlkMd(0,DeviceNumber);
		libD4000_SetBlkAd(0,DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_SetGPIORESETCOMPLETE(DeviceNumber);
		libD4000_SetEXTRESETENBL(0,DeviceNumber);
		return 1;

	}



	short libD4000_SetGPIORESETCOMPLETE(short int DeviceNumber)
	{
		return Register_Write(D4000_GPIORESETFLAG,1,DeviceNumber);
	}



	long libD4000_GetFPGARev(short int DeviceNumber)
	{
		unsigned int output = 0;
		short Upper = Register_Read(0, DeviceNumber); //Upper 16 bit version info
		short Lower = Register_Read(1, DeviceNumber); //Lower 16 bit version info
		
		//Shift both bytes from the 2 shorts into a single int
		output = Upper;
		output = output << 16;
		output = output & 0xFFFF0000; //Clear the bottom 4 bits
		output += Lower;
		return output;
	}


	
	// Used to detect if device is connected
	long libD4000_GetDriverRev(short int DeviceNumber)
	{
		// we're not using the EzUSB driver, so return a fake value
		// however, used by GUI to detect if device is connected (-1 if not)

		int result;

		if((result = open_handle(DeviceNumber)) < 0)
			return result;
		else
		{
			close_handle(DeviceNumber);
			return DMD_EZUSB_DRIVER_VERSION_FAKE;
		}
	}



	//Return a 0 for low speed, and a 1 for high speed, and a -1 for failure
	short int libD4000_GetUsbSpeed(short int DeviceNumber)
	{
		/* the original code uses the bcdUSB field in the USB device descriptor
		to determine if the bus is USB 2.0 or not.  This is not a true measure of speed,
		but one that is accessible through libusb0.1 -- true speed isn't */

		int retval, desc[14], bcdUSB;
		

		// get the descriptor using the library function that we already have
		retval = libD4000_GetDescriptor(desc, DeviceNumber);

		// return on error
		if(retval<0)
			return retval;

		// bcdUSB is in desc[2];
		bcdUSB = desc[2];


		//Assign a value in the array for each descriptor value
		if(bcdUSB == 0x0110 || bcdUSB == 0x0100) //USB 1.1 or USB 1.0
			return 0;
		else if (bcdUSB == 0x0200) //USB 2.0
			return 1;
		else //Differs from expected values
			return -3;
	}


	
	int libD4000_program_FPGA(UCHAR *write_buffer, LONG write_size, short int DeviceNumber)
	{
		int result;
		int NumberOfTransfers = 0;
		UCHAR buf[2];
		BOOL bResult;
	

		if(write_size % MAX_TRANSFER_SIZE == 0)
			NumberOfTransfers = write_size / MAX_TRANSFER_SIZE;
		else
			NumberOfTransfers = ((int)write_size / MAX_TRANSFER_SIZE) + 1;
		
		
		UCHAR (*file_buffer_swapped)[MAX_TRANSFER_SIZE] = new UCHAR[NumberOfTransfers][MAX_TRANSFER_SIZE];
	    BreakSwapBytes(file_buffer_swapped, write_buffer, &write_size, NumberOfTransfers); //Create a new array with the bytes swapped, and the transfers broken up
		
		if(open_handle(DeviceNumber) < 0)
			return 0; //USB device failed to open

		//Send code 0xBB to start FPGA program
		VendRequestOut(0xBB,DeviceNumber); //Request for FPGA program

		//Sleep(500);	// commented in original code
		
		//Send data over USB to endpoint 8 (The FPGA program endpoint)
		for(int i = 0; i < NumberOfTransfers; i++)
		{
			result = usb_bulk_write(DeviceHandle[DeviceNumber], 	// handle to device of interest
				DMD_USB_EP_FPGA_PROG | USB_ENDPOINT_OUT,	// Endpoint 8, bulk OUT FPGA programming endpoint
				(char *)file_buffer_swapped[i], 		// pointer to buffer for FPGA data
				MAX_TRANSFER_SIZE, 				// size of output buffer
				USB_TIMEOUT
				);

			if(result <= 0)
			{
				delete [] file_buffer_swapped;
				close_handle(DeviceNumber);
				return 0; //Data send failed
			}
		}


		//Wait for 0xBC01 to come back from 0xBC request to signify firmware received		
		for(int c = 0;c<10;c++) //Only try a couple times, timeout after
		{
			bResult = VendRequestIn(0xBC,buf,DeviceNumber);
			
			if(bResult == 0)
			{
				delete [] file_buffer_swapped;
				close_handle(DeviceNumber);
				return 0; //Register request failed
			}

			if(buf[0] == 0xBC && buf[1] == 0x01) //Means it received the FPGA data successfully
			{
				delete [] file_buffer_swapped;
				close_handle(DeviceNumber);
				return 1; //Return successful
			}
			else
				Sleep(200); //Wait a little bit before requesting again
		}
		delete [] file_buffer_swapped;
		close_handle(DeviceNumber);
		return 0;	//Timed out waiting for confirmation
	}



	short libD4000_LoadControl(short DeviceNumber)
	{
		short ret = 1;
		ret &= Register_Write(0x0020,0x0001,DeviceNumber); //Set the FPGA to do 1 command at a time
		ret &= Register_Write(0x0003,0x0001,DeviceNumber); //Start state
		return ret;
	}



	short libD4000_ClearFifos(short DeviceNumber)
	{
		return Register_Write(0x0003,0x0010,DeviceNumber); //Clear the FIFOs
	}



	int libD4000_LoadData(unsigned char* RowData,long length, int is1080p, short DeviceNumber) 
	{
		int NumberOfRows;
		int ret = 1;
		//ret &= libD4000_ClearFifos(DeviceNumber); //Clear the FIFOs
		
		if((BOOL)is1080p)
			NumberOfRows = length / 240; //Devide by 1080p row length
		else
			NumberOfRows = length / 128; //Devide by XGA row length
		
		ret &= Register_Write(0x0003,0x0010,DeviceNumber); //Clear the FIFOs
		
		ret &= Data_Write((UCHAR*)RowData,length,DeviceNumber);
		ret &= Register_Write(0x0020,NumberOfRows,DeviceNumber); //Set the FPGA to do NumberofRows commands
		ret &= Register_Write(0x0003,0x0001,DeviceNumber); //Start State
		return ret;		
	}



	short libD4000_SetBlkMd(short value, short DeviceNumber) 
	{
		return Register_Write(D4000_ADDR_BLKMD, value, DeviceNumber);
	}



	short libD4000_GetBlkMd(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_BLKMD, DeviceNumber);
	}



	short libD4000_SetBlkAd(short value, short DeviceNumber) 
	{
		return Register_Write(D4000_ADDR_BLKAD, value, DeviceNumber);
	}



	short libD4000_GetBlkAd(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_BLKAD, DeviceNumber);
	}



	short libD4000_SetRST2BLKZ(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_RST2BLKZ;
		else
			sCurrentVaule &= ~D4000_CTLBIT_RST2BLKZ;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetRST2BLKZ(short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_RST2BLKZ)
			return 1;
		else
			return 0;
	}



	short libD4000_SetRowMd(short value, short DeviceNumber) 
	{
		return Register_Write(D4000_ADDR_ROWMD, value, DeviceNumber);
	}



	short libD4000_GetRowMd(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_ROWMD, DeviceNumber);
	}



	short libD4000_SetRowAddr(short value, short DeviceNumber) 
	{
		return Register_Write(D4000_ADDR_ROWAD, value, DeviceNumber);
	}



	short libD4000_GetRowAddr(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_ROWAD, DeviceNumber);
	}



	short libD4000_SetSTEPVCC(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_STEPVCC;
		else
			sCurrentVaule &= ~D4000_CTLBIT_STEPVCC;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetSTEPVCC(short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_STEPVCC)
			return 1;
		else
			return 0;
	}



	short libD4000_SetCOMPDATA(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_COMPDATA;
		else
			sCurrentVaule &= ~D4000_CTLBIT_COMPDATA;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);		
	}



	short libD4000_GetCOMPDATA(short DeviceNumber) 
	{	
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_COMPDATA)
			return 1;
		else
			return 0;	
	}



	short libD4000_SetNSFLIP(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_NSFLIP;
		else
			sCurrentVaule &= ~D4000_CTLBIT_NSFLIP;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetNSFLIP( short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_NSFLIP)
			return 1;
		else
			return 0;	
	}



	short libD4000_SetWDT(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_WDT;
		else
			sCurrentVaule &= ~D4000_CTLBIT_WDT;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetWDT(short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_WDT)
			return 1;
		else
			return 0;	
	}



	short libD4000_SetPWRFLOAT(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_PWRFLOATZ;
		else
			sCurrentVaule &= ~D4000_CTLBIT_PWRFLOATZ;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetPWRFLOAT(short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_PWRFLOATZ)
			return 1;
		else
			return 0;	
	}



	short libD4000_SetEXTRESETENBL(short value, short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);

		if(value == 1)
			sCurrentVaule |= D4000_CTLBIT_EXTRESET;
		else
			sCurrentVaule &= ~D4000_CTLBIT_EXTRESET;

		return Register_Write(D4000_ADDR_CTL2, sCurrentVaule, DeviceNumber);
	}



	short libD4000_GetEXTRESETENBL(short DeviceNumber) 
	{
		short sCurrentVaule = Register_Read(D4000_ADDR_CTL2, DeviceNumber);
		if(sCurrentVaule & D4000_CTLBIT_EXTRESET )
			return 1;
		else
			return 0;	
	}



	short libD4000_SetGPIO(short value, short DeviceNumber) 
	{
		return Register_Write(D4000_ADDR_GPIO, value, DeviceNumber);
	}



	short libD4000_GetGPIO(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_GPIO, DeviceNumber);
	}



	short libD4000_GetDMDTYPE(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_DMDTYPE, DeviceNumber);
	}



	short libD4000_GetDDCVERSION(short DeviceNumber) 
	{
		return Register_Read(D4000_ADDR_DDCVERSION, DeviceNumber);
	}

}



/***********************************************
// Functions transfered over from DDC4000Ctl.cpp 
************************************************/


#ifndef LIBD4000_NO_DDC4000CTL_TRANSFER

/* Storage space for device data for each pluggable device */
DeviceData_type DeviceData[DEVICEHANDLE_NUM];



/*
// Private functions transferred from DDC4000Ctl.cpp 
*/

#define LOG_FILE stderr

/* logging service */
int DiscoveryMessageBox(short DeviceNumber, LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0)
{
	if(DeviceData[DeviceNumber].m_bAllowMessageBoxes)
	{
		fprintf(LOG_FILE, "%s\n", lpszText);
		return 1;
	}
}



short Load_DMD_Values(short DeviceNumber)
{
		//Check the DMD type and set all vars here..
	if(libD4000_GetDMDTYPE(DeviceNumber) == 0)		//1080p connected
	{
		DeviceData[DeviceNumber].DMDType = DMD_1080P;
		DeviceData[DeviceNumber].DMDWidth = 1920;
		DeviceData[DeviceNumber].DMDHeight = 1080;
		DeviceData[DeviceNumber].DMDTotalBlocks = 15;
		DeviceData[DeviceNumber].DMDRowsPerBlock = 72;
		DeviceData[DeviceNumber].DMDBytesPerRow = 240;
		DeviceData[DeviceNumber].DMDSizeinBytes = SIZE_OF_1080P;
	}
	else							// XGA connected
	{
		DeviceData[DeviceNumber].DMDType = DMD_XGA;
		DeviceData[DeviceNumber].DMDWidth = 1024;
		DeviceData[DeviceNumber].DMDHeight = 768;
		DeviceData[DeviceNumber].DMDTotalBlocks = 16;
		DeviceData[DeviceNumber].DMDRowsPerBlock = 48;
		DeviceData[DeviceNumber].DMDBytesPerRow = 128;
		DeviceData[DeviceNumber].DMDSizeinBytes = SIZE_OF_XGA;
	}

	return 1;
}




/*
// Private functions transferred from DDC4000Ctl.cpp 
*/

extern "C"
{

/* Sets specified block to all-zeros */
short libD4000_Clear(short BlockNum, short DoReset, short DeviceNumber) 
{

	libD4000_SetBlkMd(D4000_BLKMD_CLBLK, DeviceNumber);
	libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
	libD4000_SetRowAddr(0, DeviceNumber);
	BlockNum--;							//the block number comes in as 1-16, the ddc is 0-15 

	if(BlockNum <= 15)
	{
		//Clear individual block specified	
		libD4000_SetBlkAd(BlockNum, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		   
		if(DoReset)
		{
			libD4000_SetBlkMd(D4000_BLKMD_RSTBLK, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_SetGPIORESETCOMPLETE(DeviceNumber);

		}
		
	}
	else
	{
		//loop and clear entire DMD
		for(int i = 0; i < 16; i++)
		{
			libD4000_SetBlkAd(i, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			libD4000_SetBlkMd(D4000_BLKMD_CLBLK, DeviceNumber);
		}
		if(DoReset)
		{
			libD4000_SetBlkMd(D4000_BLKMD_11, DeviceNumber);
			libD4000_SetBlkAd(8, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_SetGPIORESETCOMPLETE(DeviceNumber);
		}

	}

	return 1;

}



short libD4000_FloatMirrors(short DeviceNumber)
{
	short sRtn = 1;
	sRtn &= libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
	sRtn &= libD4000_SetBlkAd(12, DeviceNumber);
	sRtn &= libD4000_SetBlkMd(D4000_BLKMD_11, DeviceNumber);
	libD4000_LoadControl(DeviceNumber);
	Sleep(1);
	sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
	libD4000_LoadControl(DeviceNumber);
	libD4000_LoadControl(DeviceNumber);
	libD4000_LoadControl(DeviceNumber);
	libD4000_LoadControl(DeviceNumber);

	return sRtn;
}


/* Load a binary image into the DMD */
/* Syntex different from DDC4000Ctl: takes a frame buffer *m_FrameBuff!!! */
short libD4000_LoadToDMD(UCHAR *m_FrameBuff, short BlockNum, short DoReset, short DeviceNumber) 
{
	short sRtn = 1;
	UINT BlockNumber;
	UCHAR *ptrOutBuffer;
	int DMDBytesPerRow = DeviceData[DeviceNumber].DMDBytesPerRow;
	int DMDRowsPerBlock = DeviceData[DeviceNumber].DMDRowsPerBlock;
	int DMDTotalBlocks = DeviceData[DeviceNumber].DMDTotalBlocks;
	int DMDType = DeviceData[DeviceNumber].DMDType;
	int BytesPerBlock = DMDBytesPerRow * DMDRowsPerBlock;
	
	
	BlockNumber = BlockNum - 1;
	

	if(BlockNumber <= 15)
	{
	
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		sRtn &= libD4000_SetRowMd(D4000_ROWMD_SET, DeviceNumber);
		sRtn &= libD4000_SetRowAddr(DMDRowsPerBlock*BlockNumber, DeviceNumber);
		ptrOutBuffer = m_FrameBuff;

		//1080p has only 15 blocks is possible that a 16th block could try to be loaded sending data after the FrameBuffer pointer.. 
		//added a check to send nothing if block 16 load is requested on the 1080p.
		if(DMDType == DMD_1080P && BlockNumber == 15)
		{
				//do nothing
		}
		else
		{
			ptrOutBuffer += BytesPerBlock*BlockNumber;
			libD4000_LoadData(ptrOutBuffer,DMDBytesPerRow, DMDType, DeviceNumber);

			Sleep(1);
			sRtn &= libD4000_SetRowMd(D4000_ROWMD_INC, DeviceNumber);
			libD4000_LoadData((ptrOutBuffer + DMDBytesPerRow),DMDBytesPerRow*(DMDRowsPerBlock - 1), DMDType, DeviceNumber);
			Sleep(1);
		}

		if(DoReset == 1)
		{
			sRtn &= libD4000_SetBlkMd(D4000_BLKMD_RSTBLK, DeviceNumber);
			sRtn &= libD4000_SetBlkAd(BlockNumber, DeviceNumber);
			sRtn &= libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_SetGPIORESETCOMPLETE(DeviceNumber);
		}


	}		
	else    //Global load
	{
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		sRtn &= libD4000_SetRowMd(D4000_ROWMD_SETPNT, DeviceNumber);
		sRtn &= libD4000_SetRowAddr(0, DeviceNumber);
		ptrOutBuffer = m_FrameBuff;
	
		libD4000_LoadData(ptrOutBuffer,DMDBytesPerRow, DMDType, DeviceNumber);

		Sleep(1);
		sRtn &= libD4000_SetRowMd(D4000_ROWMD_INC, DeviceNumber);
		libD4000_LoadData((ptrOutBuffer + DMDBytesPerRow),DMDBytesPerRow*(DMDRowsPerBlock - 1), DMDType, DeviceNumber);
		Sleep(1);
		
		
                if(DMDType == DMD_1080P)
		{
			
			for( int j = 0; j < (DMDTotalBlocks-1)/2; j++)
			{
				if( j == 0 )
				{
					ptrOutBuffer += (DMDBytesPerRow*DMDRowsPerBlock);
				}
				else
				{
					ptrOutBuffer += (DMDBytesPerRow*DMDRowsPerBlock)*2;
				}

				short temp = libD4000_LoadData(ptrOutBuffer,DMDBytesPerRow*DMDRowsPerBlock*2-2, DMDType, DeviceNumber);
				Sleep(1);
			}
		}
		
		else
		{
			for(int i = 0; i < (DMDTotalBlocks-1)/XGABLOCKSPERLOAD; i++)
			{
				if(i == 0)
				{
					ptrOutBuffer += (DMDBytesPerRow*DMDRowsPerBlock);
				}
				else
				{
					ptrOutBuffer += (DMDBytesPerRow*DMDRowsPerBlock)*XGABLOCKSPERLOAD;
				}
				libD4000_LoadData(ptrOutBuffer,DMDBytesPerRow*DMDRowsPerBlock*XGABLOCKSPERLOAD, DMDType, DeviceNumber);
				Sleep(1);
			}

		}

		if(DoReset == 1)
		{
			sRtn &= libD4000_SetBlkMd(D4000_BLKMD_11, DeviceNumber);
			sRtn &= libD4000_SetBlkAd(8, DeviceNumber);
			sRtn &= libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			Sleep(1);
			sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_LoadControl(DeviceNumber);
			libD4000_SetGPIORESETCOMPLETE(DeviceNumber);
		}			
	}

	return sRtn;
}



/* Transfer data loaded onto the DMD to the mirrors themselves */
short libD4000_Reset(short BlockNum, short DeviceNumber) 
{
	short sRtn = 1;

	if(BlockNum <= 16)
	{
		sRtn &= libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
		sRtn &= libD4000_SetBlkAd(BlockNum -1, DeviceNumber);
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		Sleep(1);
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_RSTBLK, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		Sleep(1);
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_SetGPIORESETCOMPLETE(DeviceNumber);

	}
	else
	{
		sRtn &= libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
		sRtn &= libD4000_SetBlkAd(8, DeviceNumber);
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_11, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		Sleep(1);
		sRtn &= libD4000_SetBlkMd(D4000_BLKMD_NOOP, DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_LoadControl(DeviceNumber);
		libD4000_SetGPIORESETCOMPLETE(DeviceNumber);
	}

	return sRtn;
}



short libD4000_DownloadAppsFPGACode(LPCTSTR FileName, short DeviceNumber) 
{
	short sRtn = 1; 
	FILE *fp;
	BYTE *ptConfig;
	UINT nSizeOfImage = 1569584;
	int length = 0;
	
	fp = fopen(FileName, "rb"); //open file for reading
	if(fp==NULL) //If the SrcFile is not a valid file, only set the devicenumber
	{
		DiscoveryMessageBox(DeviceNumber, _T("FPGA not configured, .bin file incorrect."));
		return 0;
	}	
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET); //go back to the beginning
	
	//Allocate file buffer.
	ptConfig = new BYTE[length+1];
	if(!ptConfig)
	{
		fclose(fp);
		DiscoveryMessageBox(DeviceNumber, _T("Error while trying to allocate .bin file buffer"));
		return 0;
	}

	fread(ptConfig,1,length,fp); //Read in the entire file
	fclose(fp);
	
	sRtn &= libD4000_program_FPGA(ptConfig, length, DeviceNumber);

	delete ptConfig;

	return sRtn;
}



/* syntex different from DDC4000Ctl: DeviceNum starts at 0 as with rest of libD4000!!! */
/* Also, SrcFile == NULL now indicates using D4000_USB_BIN */
short libD4000_ConnectDevice(short DeviceNum,LPCTSTR SrcFile) 
{

	short sRtn = 1;
	int FPGA_version = 0;

	//DeviceNumber = DeviceNum - 1; //The DLL starts with the first device as device 0


	if(SrcFile == NULL)
		SrcFile = D4000_USB_BIN;


	//Check to see if the FPGA is already configured.

	FPGA_version = libD4000_GetFPGARev(DeviceNum);
	FPGA_version &= 0xFFFF0000; //Only look at the top 16 bits, as they won't change
	if(FPGA_version == D4000_VERSION_CODE) //FPGA already programmed
	{
		sRtn &= Load_DMD_Values(DeviceNum);
		return sRtn;
	}

	sRtn &= libD4000_DownloadAppsFPGACode(SrcFile, DeviceNum);
	

	//Check the DMD type and set all vars here..
	sRtn &= Load_DMD_Values(DeviceNum);

	return sRtn;
}



/* Enable or disable error logging for specified device */
short libD4000_AllowMessages(short value, short DeviceNumber) 
{
	if(value > 0)
		DeviceData[DeviceNumber].m_bAllowMessageBoxes = TRUE;
	else
		DeviceData[DeviceNumber].m_bAllowMessageBoxes = FALSE;
	
	return 1;
}



BOOL libD4000_IsDeviceAttached(short DeviceNumber)
{
	if(libD4000_GetDriverRev(DeviceNumber) < 0)
		return FALSE;
	else
		return TRUE;
/*
	int FPGA_version;
	short numdev = libD4000_GetNumDev();

	FPGA_version = libD4000_GetFPGARev(DeviceNumber);
	FPGA_version &= 0xFFFF0000; //Only look at the top 16 bits, as they won't change
	if(FPGA_version == D4000_VERSION_CODE && DeviceNumber < numdev) //FPGA programmed, and device can possibly exist
		return TRUE;
	else
		return FALSE;
*/
}



/* This is a higher-level version of libD4000_GetRESETCOMPLETE, which sets reset to global */
short libD4000_GetResetComplete(long waittime, short DeviceNumber) 
{
	//setup registers for a Global reset
	libD4000_SetBlkMd(D4000_BLKMD_11, DeviceNumber);
	libD4000_SetBlkAd(8, DeviceNumber);
	libD4000_SetRowMd(D4000_ROWMD_NOOP, DeviceNumber);
	return libD4000_GetRESETCOMPLETE(waittime, DeviceNumber);
}



/* Returns device information.
It's up to the caller to make sure that the device is connected (libD4000_ConnectDevice) */
DeviceData_type libD4000_GetDeviceData(short DeviceNumber)
{
    return DeviceData[DeviceNumber];
}



} // extern "C"

#endif /* LIBD4000_NO_DDC4000CTL_TRANSFER */
