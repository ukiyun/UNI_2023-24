    // Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

// SET FRAME
#define FLAG 0x7E
#define ADDRESS 0x03
#define CONTROL 0x03
#define BCC1 0x03^0x03


// STATE MACHINE
#define OTHER_RCV 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5

volatile int STOP = FALSE;


int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm handler

void alarmHandler()
{
	alarmCount++;
	alarmEnabled = TRUE;

    print("Alarm #%d\n", alarmCount);
}

int main(int argc, char *argv[])
{
    // Set alarm
    (void)signal(SIGALRM, alarmHandler);
    
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    buf[0]= FLAG;
    buf[1]= ADDRESS;
    buf[2]= CONTROL;
    buf[3] = BCC1;
    buf[4]= FLAG;

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    buf[5] = '\n';

    int bytes = write(fd, buf, BUF_SIZE);
    printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    
    do{
        unsigned char buf2[BUF_SIZE+1] = {0};
        int bytes2 = read(fd, buf2,1);
        if(!bytes2) continue;
        buf2[bytes2] = '\0'; // Set end of string to '\0', so we can printf
        printf("%d bytes read\n", bytes2);

        switch (state) {
            case OTHER_RCV:
                printf("var=0x%2X\n", buf2[0]);
                if (buf2[0] == FLAG) {
                    state = FLAG_RCV;
                }
                else {
                    state = OTHER_RCV;
                }
                break;
            case FLAG_RCV:
                printf("var=0x%2X\n", buf2[0]);
                if (buf2[0] == FLAG) {
                    state = FLAG_RCV;
                }
                else if (buf2[0] == 0x01) {
                    state = A_RCV;
                }
                else {
                    state = OTHER_RCV;
                }
                break;
            case A_RCV:
                printf("var=0x%2X\n", buf2[0]);
                if (buf2[0] == FLAG) {
                    state = FLAG_RCV;
                }
                else if (buf2[0] == 0x07) {
                    state = C_RCV;
                }
                else {
                    state = OTHER_RCV;
                }
                break;
            case C_RCV:
                printf("var=0x%2X\n", buf2[0]);
                if (buf2[0] == FLAG) {
                    state = FLAG_RCV;
                }
                else if (buf2[0] == 0x01 ^ 0x07) {
                    state = BCC_OK;
                }
                else {
                    state = OTHER_RCV;
                }
            case BCC_OK:
                printf("var=0x%2X\n", buf2[0]);
                if (buf2[0] == FLAG) {
                    state = STOP;
                }
                else {
                    state = OTHER_RCV;
                }
                break;
        }

    }while (state!=STOP && alarmCount<3);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
            exit(-1);
    }

    alarm(0);
    close(fd);

    return 0;
}
