#include "vga.h"
#include "stdint.h"


VGA::VGA() :
    miscPort(0x3c2),
    crtcIP(0x3d4),
    crtcDP(0x3d5),
    seqIP(0x3c4),
    seqDP(0x3c5),
    gCIP(0x3ce),
    gCDP(0x3cf),
    aCIP(0x3c0),
    aCReadPort(0x3c1),
    aCWritePort(0x3c0),
    aCResetPort(0x3da)
{
}


VGA::~VGA()
{
}




           
void VGA::WriteRegisters(uint8_t* registers)
{
    //  misc
    miscPort.Write(*(registers++));
   
    // seq
    for(uint8_t i = 0; i < 5; i++)
    {
        seqIP.Write(i);
        seqDP.Write(*(registers++));
    }
   
    // cathode ray tube controller
    crtcIP.Write(0x03);
    crtcDP.Write(crtcDP.Read() | 0x80);
    crtcIP.Write(0x11);
    crtcDP.Write(crtcDP.Read() & ~0x80);
   
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] & ~0x80;
   
    for(uint8_t i = 0; i < 25; i++)
    {
        crtcIP.Write(i);
        crtcDP.Write(*(registers++));
    }
   
    // graphics controller
    for(uint8_t i = 0; i < 9; i++)
    {
        gCIP.Write(i);
        gCDP.Write(*(registers++));
    }
   
    // attribute controller
    for(uint8_t i = 0; i < 21; i++)
    {
        aCResetPort.Read();
        aCIP.Write(i);
        aCWritePort.Write(*(registers++));
    }
   
    aCResetPort.Read();
    aCIP.Write(0x20);
   
}


bool VGA::SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth)
{
    return width == 320 && height == 200 && colordepth == 8;
}


bool VGA::SetMode(uint32_t width, uint32_t height, uint32_t colordepth)
{
    if(!SupportsMode(width, height, colordepth))
        return false;
   
    unsigned char g_320x200x256[] =
    {
        /* MISC */
            0x63,
        /* SEQ */
            0x03, 0x01, 0x0F, 0x00, 0x0E,
        /* CRTC */
            0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
            0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
            0xFF,
        /* GC */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
            0xFF,
        /* AC */
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x41, 0x00, 0x0F, 0x00, 0x00
    };
   
    WriteRegisters(g_320x200x256);
    return true;
}

uint8_t* VGA::GetFrameBufferSegment()
{
    gCIP.Write(0x06);
    uint8_t segmentNumber = gCDP.Read() & (3<<2);
    switch(segmentNumber)
    {
        default:
        case 0<<2: return (uint8_t*)0x00000;
        case 1<<2: return (uint8_t*)0xA0000;
        case 2<<2: return (uint8_t*)0xB0000;
        case 3<<2: return (uint8_t*)0xB8000;
    }
}
           
void VGA::PutPixel(int32_t x, int32_t y,  uint8_t colorIndex)
{
    if(x < 0 || 320 <= x
    || y < 0 || 200 <= y)
        return;
       
    uint8_t* pixelAddress = GetFrameBufferSegment() + (y<<8) + (y<<6) + x;
    *pixelAddress = colorIndex;
}


uint8_t VGA::GetColorIndex(uint8_t r, uint8_t g, uint8_t b)
{
    if(r == 0x00 && g == 0x00 && b == 0x00) return 0x00; // black
    if(r == 0x00 && g == 0x00 && b == 0xA8) return 0x01; // blue
    if(r == 0x00 && g == 0xA8 && b == 0x00) return 0x02; // green
    if(r == 0xA8 && g == 0x00 && b == 0x00) return 0x04; // red
    if(r == 0xFF && g == 0xFF && b == 0xFF) return 0x3F; // white
    return 0x00;
}
           
void VGA::PutPixel(int32_t x, int32_t y,  uint8_t r, uint8_t g, uint8_t b)
{
    PutPixel(x,y, GetColorIndex(r,g,b));
}


void VGA::FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h,   uint8_t r, uint8_t g, uint8_t b)
{
    for(uint32_t Y = y; Y < y+h; Y++)
        for(uint32_t X = x; X < x+w; X++)
            PutPixel(X, Y, r, g, b);
}
