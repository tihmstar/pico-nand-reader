

#include "tusb_config.h"
#include "tusb.h"


#define USB_BCD 0x0200
#define USB_VID 0x6874
#define USB_PID 0x7064

#define MAX_PACKET_SIZE 0x40


#pragma mark usb default stuff

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x1000,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_PICO_NAND_READER,
  ITF_NUM_TOTAL
};


#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)
#define EPNUM_PICO_NAND_READER_IN      0x81
#define EPNUM_PICO_NAND_READER_OUT     0x02



uint8_t const desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_PICO_NAND_READER, 0, EPNUM_PICO_NAND_READER_OUT, EPNUM_PICO_NAND_READER_IN, CFG_TUD_VENDOR_EPSIZE),
};


// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations

  // This example use the same configuration for both high and full speed mode
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+


char const* string_desc_arr [] ={
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "tihmstar",                    // 1: Manufacturer
  "PICO_NAND_READER",            // 2: Product
  "pico-nand-reader",            // 3: Serial
};

static uint16_t _desc_str[32];
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid){
    (void) langid;
    static uint16_t _desc_str[32];
    uint8_t chr_count;

    if ( index == 0){
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {    
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) )  return NULL;

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if ( chr_count > 31 ) chr_count = 31;

            // Convert ASCII string into UTF-16
            for(uint8_t i=0; i<chr_count; i++){
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

    return _desc_str;
}
