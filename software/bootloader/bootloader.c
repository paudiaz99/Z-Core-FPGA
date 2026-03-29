#include "uart.h"

#define APP_BASE       0x00001000
#define APP_MAX_SIZE   (12 * 1024)  /* 12 KB */

#define SYNC_REQ       0x5A
#define SYNC_ACK       0xA5
#define ACK            0x06
#define NAK            0x15

/* 50 MHz / (16 * 115200) ≈ 27 */
#define BAUD_DIV_115200 27

static unsigned int recv_le32(void) {
    unsigned int v = 0;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking());
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 8;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 16;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 24;
    return v;
}

static void print_banner(void) {
    uart_puts("\r\n"
        "========================================\r\n"
        "       Z-Core RISC-V Bootloader v1.0\r\n"
        "========================================\r\n"
        " CPU\r\n"
        "   ISA      : RV32IM + Zicsr\r\n"
        "   Clock    : 50 MHz\r\n"
        "   Pipeline : 5-stage\r\n"
        " Memory\r\n"
        "   RAM      : 16 KB @ 0x00000000\r\n"
        "   Boot     : 0x0000-0x0FFF (4 KB)\r\n"
        "   App      : 0x1000-0x3FFF (12 KB)\r\n"
        " Peripherals\r\n"
        "   UART     : 0x04000000  115200 8N1\r\n"
        "   GPIO     : 0x04001000\r\n"
        "   Timer    : 0x04002000\r\n"
        "   VGA      : 0x04003000  160x120\r\n"
        "========================================\r\n"
        "Waiting for upload...\r\n");
}

void main(void) {
    uart_set_baud(BAUD_DIV_115200);

    print_banner();

    /* ---- Sync handshake ---- */
    while ((unsigned char)uart_getc_blocking() != SYNC_REQ)
        ;
    uart_putc((char)SYNC_ACK);

    /* ---- Receive payload size (4 bytes, little-endian) ---- */
    unsigned int size = recv_le32();

    if (size == 0 || size > APP_MAX_SIZE) {
        uart_putc((char)NAK);
        uart_puts("ERR: bad size ");
        uart_putint((int)size);
        uart_puts("\r\n");
        while (1)
            ;
    }

    uart_putc((char)ACK);
    uart_puts("RX ");
    uart_putint((int)size);
    uart_puts(" bytes\r\n");

    /* ---- Receive data ---- */
    unsigned char *dest = (unsigned char *)APP_BASE;
    unsigned int checksum = 0;

    for (unsigned int i = 0; i < size; i++) {
        unsigned char b = (unsigned char)uart_getc_blocking();
        dest[i] = b;
        checksum += b;
    }

    /* ---- Verify checksum ---- */
    unsigned int expected = recv_le32();

    if (checksum != expected) {
        uart_putc((char)NAK);
        uart_puts("ERR: checksum ");
        uart_puthex(checksum);
        uart_puts(" != ");
        uart_puthex(expected);
        uart_puts("\r\n");
        while (1)
            ;
    }

    uart_putc((char)ACK);
    uart_puts("OK! Jumping to ");
    uart_puthex(APP_BASE);
    uart_puts("\r\n");

    /* Wait for UART TX to finish */
    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;

    /* Jump to loaded application */
    void (*app)(void) = (void (*)(void))APP_BASE;
    app();
}
