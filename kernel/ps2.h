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
    int mouse_x;
    int mouse_y;
    bool left_button_active;
    bool right_button_active;
    bool middle_button_active;

    friend class VGA;

public:
    PS2Controller() : port(0x60), status(0), output(0), mouse_x(0), mouse_y(0), left_button_active(false),  right_button_active(false),
        middle_button_active(false) {}

    void initialize() {

        
        //disables Devices
        outb(0x64, 0xAD);
        outb(0x64, 0xA7);

        //flush the output buffer by reading port 0x60 without testing bit 0
        inb(0x60);

        //enable the device
        outb(0x64, 0xAE);
        outb(0x64, 0xA8);



        //enable mouse
        outb(0x64, 0xD4);                    // tell the controller to address the mouse
        outb(0x60, 0xF4);                    // write the mouse command code to the controller's data port
        // while(!(inb(0x64) & 1) asm("pause"); // wait until we can read
        // ack = inb(0x60);                     // read back acknowledge. This should be 0xFA
        // outb(0xD4, 0x64);                    // tell the controller to address the mouse
        // outb(100, 0x60);                     // write the parameter to the controller's data port
        // while(!(inb(0x64) & 1) asm("pause"); // wait until we can read
        // ack = inb(0x60);                     // read back acknowledge. This should be 0xFA
    }

    int poll(){
       // To poll, wait until bit 0 of the Status Register becomes set, then read the received byte of data from IO Port 0x60.
       Debug::printf("in poll\n");
       int byte = inb(0x64);
        while(!((byte & 0x1) == 1)){
            byte = inb(0x64);
        }

        //unsure of this, having it as byte for now. depends how poll is used.
        Debug::printf("poll works\n");
        return inb(0x60);
    }

    // int readByte() {
    //     unsigned char byte;
    //     // while ((inb(port + 1) & 1) == 0) {
    //     //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     // }
    //     byte = inb(port);
    //     if (byte == 0xFA) {
    //         return -1; // ACK
    //     } else if (byte == 0xFE) {
    //         return -2; // RESEND
    //     } else if (byte == 0xEE) {
    //         return -3; // ERROR
    //     } else {
    //         return byte;
    //     }
    // }

    void update() {
        int bytes = poll();
        if((bytes & 0x1) == 1){
            left_button_active = !left_button_active;
            Debug::printf("left mouse clicked\n\n");
        }

        else if(((bytes >>1 ) & 0x1) == 1){
            right_button_active = !right_button_active;
            Debug::printf("right mouse clicked\n\n");
        }
        else if(((bytes >> 2) & 0x1) == 1){
            middle_button_active = !middle_button_active;
            Debug::printf("middle mouse clicked\n\n");
        }else{}

        int state = bytes & 0xFF;
        int8_t x = poll();
        int8_t rel_x = x -((state << 4) & 0x100);
        int8_t y = poll();
        int8_t rel_y = y - ((state << 3) & 0x100);

        mouse_x = mouse_x + rel_x;
        mouse_y = mouse_y + rel_y;

        // mouse_x = mouse_x + x;
        // mouse_y = mouse_y + y;

        Debug::printf("x coord: %d\n", mouse_x);
        Debug::printf("y coord: %d\n", mouse_y);
        
        
        
        // while(bytes == 0xFA) bytes = inb(port);
        // int buttons = bytes & 0xFF; // left button & 0x1, right button & 0x2, middle button & 
        // int mouse_packet_rel_x = (bytes >> 8) & 0xFF;
        // int mouse_packet_rel_y = (bytes >> 16) & 0xFF;
        // mouse_x = mouse_x + mouse_packet_rel_x;
        // mouse_y = mouse_y + mouse_packet_rel_y;




    }
};
