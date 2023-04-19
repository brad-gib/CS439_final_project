#include "usb.h"
#include "stdint.h"
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

class Port {
    uint16_t portnum;

    public: 
        Port(uint16_t portnum) : portnum(portnum) {}
        // ~Port();

        void write(uint8_t data) {
            write8(portnum, data);
        }

        uint8_t read() {
            return read8(portnum);
        }

        static inline uint8_t read8(uint16_t _port) {
            uint8_t result;
            __asm__ volatile("inb %1, %0" : "=a" (result) : "Nd" (_port));
            return result;
        }

        static inline void write8(uint16_t _port, uint8_t _data) {
            __asm__ volatile("outb %0, %1" :  : "a" (_data), "Nd" (_port));
        }
};

class VGA {
    Port miscPort;
    Port crtcIP;
    Port crtcDP;
    Port seqIP;
    Port seqDP;
    Port gCIP;
    Port gCDP;
    Port aCIP;
    Port aCRP;
    Port aCWP;
    Port aCResetP;
    
    void WriteRegisters(uint8_t* registers);
    uint8_t* GetFrameBufferSegment();

    virtual uint8_t getColorIndex(uint8_t r, uint8_t g, uint8_t b);

    public:
        VGA();
        // virtual ~VGA();

        virtual bool SetMode(uint32_t width, uint32_t height, uint32_t colordepth);
        virtual bool SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth);
        virtual void updatePixel(uint32_t x, uint32_t y, uint8_t colorIndex);
        virtual void PutPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b); // 24 bit color code (r, g, b)
        virtual void FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b);
};

