#include "usb.h"
#include "idt.h"
#include "debug.h"
#include "threads.h"
#include "process.h"
#include "machine.h"
#include "ext2.h"
#include "elf.h"
#include "libk.h"
#include "file.h"
#include "heap.h"
#include "shared.h"
#include "kernel.h"
#include "stdint.h"
#include "port.h"


class VGA
{
protected:
    Port8Bit miscPort;
    Port8Bit crtcIP;
    Port8Bit crtcDP;
    Port8Bit seqIP;
    Port8Bit seqDP;
    Port8Bit gCIP;
    Port8Bit gCDP;
    Port8Bit aCIP;
    Port8Bit aCReadPort;
    Port8Bit aCWritePort;
    Port8Bit aCResetPort;
   
    void WriteRegisters(uint8_t* registers);
    uint8_t* GetFrameBufferSegment();
   
    virtual uint8_t GetColorIndex(uint8_t r, uint8_t g, uint8_t b);
   
   
public:
    VGA();
    ~VGA();
   
    virtual bool SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth);
    virtual bool SetMode(uint32_t width, uint32_t height, uint32_t colordepth);
    virtual void PutPixel(int32_t x, int32_t y,  uint8_t r, uint8_t g, uint8_t b);
    virtual void PutPixel(int32_t x, int32_t y, uint8_t colorIndex);
   
    virtual void FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h,   uint8_t r, uint8_t g, uint8_t b);

    // void vga_set_cursor_pos(uint8_t x, uint8_t y) {
    //     // The screen is 80 characters wide...
    //     uint16_t cursorLocation = (y<<8) + (y<<6) + x;
    //     outb(0x3D4, 14);
    //     outb(0x3D5, cursorLocation >> 8);
    //     outb(0x3D4, 15);
    //     outb(0x3D5, cursorLocation);
    // }


};
