/*
 * Extracted from jstest.c developed by Vojtech Pavlik
 */

/*
 * This program can be used to test all the features of the Linux
 * joystick API. It is also intended to serve as an example 
 * implementation for those who wish to learn
 * how to write their own joystick using applications.
 */


#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <linux/input.h>
#include <linux/joystick.h>

#define NAME_LENGTH 128

int main (int argc, char **argv)
{

	struct ff_effect effect, effect_strong, effect_weak;
	struct input_event gain, playR, playL, stop;
	int fd, fd_vib;
	unsigned char axes = 2;
	unsigned char buttons = 2;
	int version = 0x000800;
	char name[NAME_LENGTH] = "Unknown";
	const char * device_file_name = "/dev/input/event6";
	fd_vib = open(device_file_name, O_RDWR);

	if (argc < 2 || argc > 3 || !strcmp("--help", argv[1])) {
		puts("");
		puts("Usage: jstest [<mode>] <device>");
		puts("");
		puts("");
		exit(1);
	}
	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
		perror("jstest");
		exit(1);
	}

	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &buttons);
	ioctl(fd, JSIOCGNAME(NAME_LENGTH), name);

	printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
		name, axes, buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);
	printf("Testing ... (interrupt to exit)\n");

/*
 * Event interface, single line readout.
 */
	memset(&gain, 0, sizeof(gain));
	gain.type = EV_FF;
	gain.code = FF_GAIN;
	gain.value = 0xFFFF; /* [0, 0xFFFF]) */
	memset(&effect_strong,0,sizeof(effect));

	effect_strong.type = FF_RUMBLE;
	effect_strong.id = -1;
	effect_strong.u.rumble.strong_magnitude = 0xFFFF;
	effect_strong.u.rumble.weak_magnitude = 0;
	effect_strong.replay.length = 100; // 900 ms
	effect_strong.replay.delay = 0;

	printf("Uploading effect # (Strong rumble, with heavy motor) ... ");
	fflush(stdout);
	if (ioctl(fd_vib, EVIOCSFF, &effect_strong) == -1) {
		perror("Error");
	} else {
		printf("OK (id %d)\n", effect_strong.id);
	}

	memset(&effect_weak,0,sizeof(effect));
	effect_weak.type = FF_RUMBLE;
	effect_weak.id = -1;
	effect_weak.u.rumble.strong_magnitude = 0;
	effect_weak.u.rumble.weak_magnitude = 0xFFFF;
	effect_weak.replay.length =  100; // 900 ms
	effect_weak.replay.delay = 0;

	printf("Uploading effect # (Weak rumble) ... ");
	fflush(stdout);
	if (ioctl(fd_vib, EVIOCSFF, &effect_weak) == -1) {
		perror("Error:");
	} else {
		printf("OK (id %d)\n", effect_weak.id);
	}

	if (argc == 2 ) {

		int *axis;
		int *button;
		int i;
		struct js_event js;

		axis = calloc(axes, sizeof(int));
		button = calloc(buttons, sizeof(char));

		while (1) {
			if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
				perror("\njstest: error reading");
				exit (1);
			}

			switch(js.type & ~JS_EVENT_INIT) {
			case JS_EVENT_BUTTON:
				button[js.number] = js.value;
				break;
			case JS_EVENT_AXIS:
				axis[js.number] = js.value;
				break;
			}

			printf("\r");

			if (axes) {
				printf("Axes: ");
				for (i = 0; i < axes; i++)
					printf("%2d:%6d ", i, axis[i]);
			}

			if (buttons) {
				memset(&playR,0,sizeof(playR));
				playR.type = EV_FF;
				playR.code = effect_weak.id;
				playR.value = 8;
				memset(&playL,0,sizeof(playL));
				playL.type = EV_FF;
				playL.code = effect_strong.id;
				playL.value = 8;
				printf("Buttons: ");
				for (i = 0; i < buttons; i++)
					printf("%2d:%s ", i, button[i] ? "on " : "off");
				if(button[4]){
					effect_strong.u.rumble.strong_magnitude -= 0x0FFF;
					ioctl(fd_vib, EVIOCSFF, &effect_strong);
					write(fd_vib, (const void*) &playL, sizeof(playL));
				}
				if(button[5]){
					effect_strong.u.rumble.strong_magnitude += 0x0FFF; 
					ioctl(fd_vib, EVIOCSFF, &effect_strong);
					write(fd_vib, (const void*) &playL, sizeof(playL));
				}
				if(button[0]){write(fd_vib, (const void*) &playR, sizeof(playR));}
			}

			fflush(stdout);
		}
	}


	return -1;
}
