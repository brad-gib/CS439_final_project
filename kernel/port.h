#include "stdint.h"
class Port
{
    protected:
        Port(uint16_t portnumber);
        // FIXME: Must be virtual (currently isnt because the kernel has no memory management yet)
        ~Port();
        uint16_t portnumber;
};



class Port8Bit : public Port
{
    public:
        Port8Bit(uint16_t portnumber);
        ~Port8Bit();


        virtual uint8_t Read();
        virtual void Write(uint8_t data);


    protected:
        static inline uint8_t Read8(uint16_t _port)
        {
            uint8_t result;
            __asm__ volatile("inb %1, %0" : "=a" (result) : "Nd" (_port));
            return result;
        }


        static inline void Write8(uint16_t _port, uint8_t _data)
        {
            __asm__ volatile("outb %0, %1" : : "a" (_data), "Nd" (_port));
        }
};
