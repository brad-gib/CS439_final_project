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
#include "vga.h"

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
    int colors[5][3];
    VGA* vga;
    int counter;
    int size;
    friend class VGA;


public:
    PS2Controller(VGA* vga) : port(0x60), status(0), output(0), mouse_x(0), mouse_y(0), left_button_active(false),  right_button_active(false),
        middle_button_active(false), vga(vga), counter(0), size(1) {
            colors[0][0] = 0x00;
            colors[0][1] = 0x00;
            colors[0][2] = 0x00;

            colors[1][0] = 0x00;
            colors[1][1] = 0x00;
            colors[1][2] = 0xA8;

            colors[2][0] = 0x00;
            colors[2][1] = 0xA8;
            colors[2][2] = 0x00;

            colors[3][0] = 0xA8;
            colors[3][1] = 0x00;
            colors[3][2] = 0x00;

            colors[4][0] = 0xFF;
            colors[4][1] = 0xFF;
            colors[4][2] = 0xFF;
            // colors = { {0x00, 0x00, 0x00}, // black
            //            {0x00, 0x00, 0xA8}, // blue
            //            {0x00, 0xA8, 0x00}, // green
            //            {0xA8, 0x00, 0x00}, // red
            //            {0xFF, 0xFF, 0xFF}, // white
            //          };
        }

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
    //    Debug::printf("in poll\n");
       int byte = inb(0x64);
        while(!((byte & 0x1) == 1)){
            byte = inb(0x64);
        }

        //unsure of this, having it as byte for now. depends how poll is used.
        // Debug::printf("poll works\n");
        return inl(0x60);
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
        int byte1 = bytes & 0xFF;
        int byte2 = (bytes & 0xFF00) >> 8;
        int byte3 = (bytes & 0xFF0000) >> 16;
        int byte4 = (bytes & 0xFF000000) >> 24;
        // Debug::printf("byte1: %x, byte2: %x, byte3: %x, byte4: %x\n", byte1, byte2, byte3, byte4);

        if(bytes & 0b00001000 && (!(byte1 == byte2 && byte1 == byte3 && byte1 == byte4))){
            // Debug::printf("mouse input\n");
            mouseUpdate(bytes);
        } else{
            keyboardUpdate(bytes);
        }
    }

    void keyboardUpdate(int bytes){
        // Debug::printf("keyboard input\n");
        int first_byte = bytes & 0xFF;
        int byte2 = (bytes & 0xFF00) >> 8;
        int byte3 = (bytes & 0xFF0000) >> 16;
        int byte4 = (bytes & 0xFF000000) >> 24;
        if(!(first_byte == byte2 && first_byte == byte3 && first_byte == byte4)){
            return;
        }
        if(first_byte == 0x39){
            //space bar pressed
            // Debug::printf("clearing %x\n", bytes);
            vga->FillRectangle(0,0,320,200,0xFF,0xFF,0xFF);
        } else if(first_byte == 0x2E){
            //'c' pressed
            counter++;
            if(counter >= 5) counter = 0;
        } else if(first_byte == 0x16){
            //'u' pressed
            if(size < 30){
                size++;
            }
        } else if(first_byte == 0x20){
            //'d' pressed
            if(size > 1){
                size--;
            }
        } else if(first_byte == 0x21){
            //'f' pressed
            vga->FillRectangle(0,0,320,200,colors[counter][0],colors[counter][1],colors[counter][2]);
        } else if(first_byte == 0x02){
            //'1' pressed
            for(int i = 0; i < 15; i ++){
                int index = i%5;
                vga->FillRectangle(0,0,320,200,colors[index][0],colors[index][1],colors[index][2]);
            }
        }
    }

    void mouseUpdate(int bytes){
        if((bytes & 0x1) == 1){
            left_button_active = !left_button_active;
            if(left_button_active){
                vga->FillRectangle(mouse_x,mouse_y,size,size,colors[counter][0],colors[counter][1],colors[counter][2]);
            } 
            // Debug::printf("left mouse clicked\n\n");
        }

        // else if(((bytes >>1 ) & 0x1) == 1){
        //     right_button_active = !right_button_active;
        //     if(right_button_active){
        //         vga->FillRectangle(0,0,320,200,0xFF,0xFF,0xFF);
        //     }
        //     // Debug::printf("right mouse clicked\n\n");
        // }
        // else if(((bytes >> 2) & 0x1) == 1){
        //     middle_button_active = !middle_button_active;
        //     counter++;
        //     if(counter >= 5) counter = 0;
        //     // Debug::printf("middle mouse clicked\n\n");
        // }

        int state = bytes & 0xFF; // first byte
        int x = (bytes >> 8) & 0xFF; // second byte
        int32_t rel_x = x -((state << 4) & 0x100);
        int y = (bytes >> 16) & 0xFF; // third byte
        int32_t rel_y = y - ((state << 3) & 0x100);


        // Debug::printf("x: %x, y: %x\n", x, y);


        if((mouse_x + rel_x < 320) && (mouse_x + rel_x >= 0)){
            mouse_x = mouse_x + rel_x;
        }
       
        if((mouse_y - rel_y < 200) && (mouse_y - rel_y >= 0)){
            mouse_y = mouse_y - rel_y;
        }

        // Debug::printf("x: %d, y: %d\n", mouse_x, mouse_y);

        // while(bytes == 0xFA) bytes = inb(port);
        // int buttons = bytes & 0xFF; // left button & 0x1, right button & 0x2, middle button & 
        // int mouse_packet_rel_x = (bytes >> 8) & 0xFF;
        // int mouse_packet_rel_y = (bytes >> 16) & 0xFF;
        // mouse_x = mouse_x + mouse_packet_rel_x;
        // mouse_y = mouse_y + mouse_packet_rel_y;
    }
};
