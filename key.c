#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "gpio_lib.h"

#define PID_FILE "/var/run/gpiokey/gpiokey.pid"

#define LED1	SUNXI_GPE(4)
#define LED2	SUNXI_GPE(5)
#define LED3	SUNXI_GPE(6)
#define LED4	SUNXI_GPE(7)
#define LED5	SUNXI_GPE(8)
#define LED6	SUNXI_GPE(9)
#define LED7	SUNXI_GPE(10)
#define LED8	SUNXI_GPE(11)

#define KEY0	SUNXI_GPI(9)
#define KEY1	SUNXI_GPI(8)
#define KEY2	SUNXI_GPI(7)

int Daemon(void);
int init();
int readinput(int pin);
int ledon(int pin);
int ledoff(int pin);
int LEDS[8] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};
int LEDSS[8];
int KEYS[3] = {KEY0, KEY1, KEY2};
int KEYSP[3] = {0, 0, 0};

void SetPidFile(char* Filename){
    FILE* f;
    f = fopen(Filename, "w+");
    if (f) {
	fprintf(f, "%u", getpid());
	fclose(f);
    }
}

int init() {
    SetPidFile(PID_FILE);
    int l;
    openlog("GPIO_KEY",LOG_PID,LOG_USER);
    if(SETUP_OK!=sunxi_gpio_init()){
	syslog(LOG_INFO,"Error - Failed to initialize GPIO\n");
	return -1;
    }
    for (l=0; l<3; ++l) {
	if(SETUP_OK!=sunxi_gpio_set_cfgpin(KEYS[l],SUNXI_GPIO_INPUT)){
	    syslog(LOG_INFO,"Error - Failed to initialize GPIO\n");
	    return -1;
	};
	if(SETUP_OK!=sunxi_gpio_set_pull(KEYS[l],UP)){
	    syslog(LOG_INFO,"Error - Failed to PULL set GPIO\n");
	    return -1;
	};
    }
    for (l=0; l<8; ++l) {
	if(SETUP_OK!=sunxi_gpio_set_cfgpin(LEDS[l],SUNXI_GPIO_OUTPUT)){
	    syslog(LOG_INFO,"Error - Failed to initialize GPIO\n");
	    return -1;
	};
    }
    for (l=0; l<8; ++l) {
	ledoff(LEDS[l]);
	LEDSS[l] = 0;
    }
    syslog(LOG_INFO,"Init OK");
}

int readinput(int pin) {
    unsigned char buffer = 0xAA;
    unsigned char timeout = 200;
    while( timeout-- > 0 ) {
	buffer <<= 1;
	buffer |= (sunxi_gpio_input(pin) ? 0x01 : 0x00);
	if (buffer == 0xFF) return 1;
	if (buffer == 0x00) return 0;
	usleep(100);
    }
    return 0;
}

int ledon(int pin) {
    if(sunxi_gpio_output(pin,HIGH)){
	syslog(LOG_INFO,"Error - Failed to set GPIO pin value\n");
        return -1;
    }
    return 0;
}

int ledoff(int pin) {
    if(sunxi_gpio_output(pin,LOW)){
	syslog(LOG_INFO,"Error - Failed to set GPIO pin value\n");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    pid_t parpid, sid;
    parpid = fork();
    if(parpid < 0) {
        exit(1);
    } else if(parpid != 0) {
        exit(0);
    }
    umask(0);
    sid = setsid();
    if(sid < 0) {
        exit(1);
    }
    if((chdir("/")) < 0) {
        exit(1);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return Daemon();
}

int Daemon(void){
    int t;
    init();
     while(1){
	for (t=0; t<3; ++t) {
	    if(!readinput(KEYS[t])) {
		if (!KEYSP[t]) {
		    if (!LEDSS[t]) {
			ledon(LEDS[t]); 
			LEDSS[t] = 1;
			syslog(LOG_INFO,"LED%d - On",t+1);
		    } else {
			ledoff(LEDS[t]);
			LEDSS[t] = 0;
			syslog(LOG_INFO,"LED%d - Off",t+1);
		    }
		    KEYSP[t] = 1;
		}
	    } else {
		KEYSP[t] = 0;
	    }
	}
    }

    sunxi_gpio_cleanup();
    closelog();
    return 0;
}