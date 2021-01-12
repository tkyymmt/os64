#include "asm.h"
#include "kbc.h"


#define KBC_DATA_ADDR       0x0060
#define KBC_DATA_BIT_BRAKE  0x80
#define KBC_STATUS_ADDR     0x0064
#define KBC_STATUS_BIT_OBF  0x01


static const char get_kbc_data()
{
    // wait until KBC_STATUS_BIT_OBF bit is set
    while (!(inb(KBC_STATUS_ADDR) & KBC_STATUS_BIT_OBF));

    return inb(KBC_DATA_ADDR);
}

const char get_keycode()
{
    uint8_t keycode;

    // wait until KBC_DATA_BIT_BRAKE bit is off
    while ((keycode = get_kbc_data()) & KBC_DATA_BIT_BRAKE)

    return keycode;
}