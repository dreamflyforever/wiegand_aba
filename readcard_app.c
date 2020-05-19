#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

unsigned int fd;
unsigned int key_card_number[3] ={ 0,0,0,};

void sig_handler()
{
	unsigned int i;
	unsigned int ret;
	unsigned int val;
	ret = read(fd, key_card_number, 3 * sizeof(int));

	if (ret < 0) {
		printf("read err!\n");
		return 0;
	} 

	if (((key_card_number[0] & 0xf) == 0xb) && key_card_number[1]) {
		for(i = 0; i < 3; i++) {
			if(key_card_number[i] == 0) {
				return 0;
			}
			printf("ABA_card application printf key_card_number[%d]: %x\n",
				i, key_card_number[i]); 
		}
	} else {
		if (key_card_number[0] & 0x80000000) {
			key_card_number[0] <<= 1;
			key_card_number[0] >>= 1;
			printf("weigen char_front :%d\n",key_card_number[0] >> 16);
			printf("weigen short_after : %d\n",(key_card_number[0] & 0xffff));
			printf("weigen application printf key_card_number: %d%d\n",
				key_card_number[0] >> 16, (key_card_number[0] & 0xffff));
		} else {
			printf("ABA_kye application printf key_card_number[0]: %x\n",
				key_card_number[0]);
		}
	}
	memset(key_card_number, 0, 3 * sizeof(int));
}

int main(int argc, char **argv)
{  
	int ret;

	int f_flags;
	fd = open("/dev/key_read_device", O_RDWR);        
	if (fd < 0) {
		printf("Can't open /dev/key_read_device\n");
		perror("/dev/key_read_device:");
		return -1;
	}
	signal(SIGIO, sig_handler);
	fcntl(fd, F_SETOWN, getpid());
	f_flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, f_flags | FASYNC);
	printf("key_read_device is = %d\n", fd);
	
	while (1) {
		sleep(10);
		printf("sleep 1000\n");
	}

	close(fd);
	return 0;    
}
