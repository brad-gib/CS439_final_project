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


class PS2Controller {

private:
    unsigned short port;  //the port that 
    int status;
    int output;

public:
    PS2Controller() : port(0x60), status(0), output(0) {}

    bool initialize() {

        status = inb(port + 1);
        status &= ~0x20;
        status |= 0x10;
        outb(status, port + 1);
        outb(0xF4, port);
        output = readByte();
        return true;
    }

    int readByte() {
        unsigned char byte;
        // while ((inb(port + 1) & 1) == 0) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // }
        byte = inb(port);
        if (byte == 0xFA) {
            return -1; // ACK
        } else if (byte == 0xFE) {
            return -2; // RESEND
        } else if (byte == 0xEE) {
            return -3; // ERROR
        } else {
            return byte;
        }
    }
}