#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include <linux/irqflags.h>
#include <linux/gpio.h>

#define KEY0         0x00
#define KEY1         0x01
#define KEY2         0x02
#define KEY3         0x03
#define KEY4         0x04
#define KEY5         0x05
#define KEY6         0x06
#define KEY7         0x07
#define KEY8         0x08
#define KEY9         0x09
#define KEY10        0x0a
#define KEY11        0x0b
#define KEY_MINOR    252
#define ms           15    
#define init_entity  60    
#define data   S3C2410_GPF2
#define clock  S3C2410_GPF3 
#define DEVICE_NAME "key_read_device"

typedef struct key_irq_desc {
	int            irq;  
	unsigned long  flags; 
	char           *name;          
	unsigned int   id;
} key_irq_desc;

typedef struct key_card_struct {

	unsigned int flags;         /*key number*/

	unsigned int interrupt_times;/*interrupt times*/

	unsigned int number[3];     /*store card number*/

	unsigned int one_number;    /*flag for total of 1 of effective end*/

	unsigned int times_after;  /*check end-read for last 5 times*/

	unsigned int flags_after;   /*flag for end-read*/

	unsigned int flags_card_end; /*number for  effective card */

	unsigned int interrupt_flags;/*preamble end, record interrupt times*/

	unsigned int start;          /*start flag*/

	unsigned int odd_one_number; /*odd sum*/

	unsigned int odd_start;      /*check sum flag*/

	unsigned int verify_number;  /*when sum equil to 5, then odd add 1*/

	unsigned int data_interrupt_times ;/*data bus interrupt times*/

	struct timer_list card_timer;  /*timer*/

	struct fasync_struct *async_queue;

	spinlock_t card_lock;

} key_card_struct;

typedef struct wiegand_key_card {
	unsigned int char_front; /*befor two bytes*/
	unsigned int short_next; /*after two bytes*/
	unsigned int odd;        /*after odd check, 12bit*/
	unsigned int even;       /*befor even check*/
	unsigned int number;     /*wiegand key*/
} wiegand_key_card;

key_irq_desc key_irqs[] = {
	{IRQ_EINT3, IRQF_TRIGGER_FALLING, "DATA6", 5},  /*clock*/
	{IRQ_EINT2, IRQF_TRIGGER_FALLING, "DATA5", 6}, /*data*/ 
}; 

key_card_struct key_card_entity = {
	.flags           = 0,
	.interrupt_times = 0,
	.number[0]       = 0,
	.number[1]       = 0,
	.number[2]       = 0,
	.one_number      = 0,
	.flags_after     = 0,
	.times_after     = 0,
	.flags_card_end  = 0,
	.interrupt_flags = 0,
	.interrupt_flags = 0,
	.start           = 0,
	.odd_one_number  = 0,
	.odd_start       = 0,  
	.data_interrupt_times = 0,
};

wiegand_key_card wiegand_entity = {0, 0, 0, 0, 0};

void interrupt_next(unsigned long);
void card_timer_func(unsigned long expires);

DECLARE_TASKLET(card_tasklet,interrupt_next,0);

void card_timer_func(unsigned long expires)
{
	unsigned int flags_err_int = 0;
	printk("card_timer_func show cloc_interrupt_times is %d\n",
		key_card_entity.interrupt_times);
	spin_lock(&(key_card_entity.card_lock));
	switch (key_card_entity.interrupt_times)
	{
		case 12:
			key_card_entity.flags = KEY0;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);            
			break;

		case 15:
			key_card_entity.flags = KEY1;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);     
			break;

		case 11:
			key_card_entity.flags = KEY2;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 7:
			key_card_entity.flags = KEY3;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);      
			break;

		case 14:
			key_card_entity.flags = KEY4;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 10:
			key_card_entity.flags = KEY5;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);     
			break;

		case 6:
			key_card_entity.flags = KEY6;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 13:  
			key_card_entity.flags = KEY7;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 9:     
			key_card_entity.flags = KEY8;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 5:
			key_card_entity.flags = KEY9;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 16:
			key_card_entity.flags = KEY10;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		case 8:
			key_card_entity.flags = KEY11;
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
			break;

		default:
			flags_err_int = 1;
			break;     
	}
	switch (key_card_entity.interrupt_times + key_card_entity.data_interrupt_times)
	{
		case 4:
			flags_err_int = 0;
			printk("interrupt_times  = 4\n");
			tasklet_schedule(&card_tasklet);
			break;

		case 26:
			flags_err_int = 0;
			printk("interrupt_times  = 26\n");
			tasklet_schedule(&card_tasklet);
			break;
		default:
			break;
	}
	if (flags_err_int) {
		if (!((key_card_entity.number[0] & 0xf) == 0xb)) {

			memset(&key_card_entity,0,init_entity);

			memset(&wiegand_entity, 0, sizeof(wiegand_entity));  
		}    
	}
	spin_unlock(&(key_card_entity.card_lock));
	printk("data_interrupt times is :%d\n",key_card_entity.data_interrupt_times);
}

void interrupt_next(unsigned long omitted)
{   
	unsigned int i;
	unsigned int wiegand_num_copy;
	printk("interrupt_next\n");
	if ((key_card_entity.interrupt_times +
		key_card_entity.data_interrupt_times) == 4) {  
		printk("interrupt_times == 4\n");
		printk("printk:%x\n",wiegand_entity.number);                  
		kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);   
		return;
	}

	wiegand_num_copy = wiegand_entity.number;

	if ((key_card_entity.interrupt_times +
		key_card_entity.data_interrupt_times ) == 26) {

		for (i = 1; i <= 26; i++) {   
			if (i <= 13) {
				if (wiegand_num_copy & 0x1)
					wiegand_entity.odd++; 
				wiegand_num_copy >>= 1;
			}
			if(i >= 14) {
				if (wiegand_num_copy & 0x1)
					wiegand_entity.even++;
				wiegand_num_copy >>= 1;
			}
		}

		if ((wiegand_entity.even % 2) == 0 ) {
			printk("even verify success\n");     
		} else {
			printk("even verify error\n"); 

			memset(&key_card_entity,0,init_entity);
			memset(&wiegand_entity, 0, sizeof(wiegand_entity));
			return ;
		}      

		if ((wiegand_entity.odd % 2) == 1 ) {
			printk("odd verify success\n");
		} else {
			printk("odd verify error\n");

			memset(&key_card_entity,0,init_entity);

			memset(&wiegand_entity, 0, sizeof(wiegand_entity));
			return ;
		}

		printk("wiegand_entity.number is:");
		wiegand_entity.char_front = (wiegand_entity.number << 7) & 0xff000000;
		wiegand_entity.char_front = wiegand_entity.char_front >> 24;
		printk("%d",wiegand_entity.char_front);       

		wiegand_entity.short_next = wiegand_entity.number & 0x1fffe;
		wiegand_entity.short_next = wiegand_entity.short_next >> 1;
		printk("%d\n",wiegand_entity.short_next);

		wiegand_entity.number = (wiegand_entity.char_front << 16) | wiegand_entity.short_next;
		wiegand_entity.number |= 0x80000000; 

		kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);        
	}
}


static irqreturn_t data_interrupt(int irq, void *dev_id)
{       
	local_irq_disable();
	key_card_entity.data_interrupt_times++;
	if (key_card_entity.data_interrupt_times == 1) { 
		mod_timer(&key_card_entity.card_timer,jiffies + ms);
	}

	/*key_card_entity.interrupt_times++;*/
	wiegand_entity.number = wiegand_entity.number << 1;  

	local_irq_enable();  
	return IRQ_HANDLED;
}


static irqreturn_t clock_interrupt(int irq, void *dev_id)
{
	local_irq_disable();

	key_card_entity.interrupt_times++; 


	if ( key_card_entity.interrupt_times == 1) { 
		mod_timer(&key_card_entity.card_timer,jiffies + ms); 
	}

	if (key_card_entity.interrupt_times < 
		(33 + key_card_entity.interrupt_flags)) {
		if ( !s3c2410_gpio_getpin(data)) {

			key_card_entity.number[0] = key_card_entity.number[0] >> 1;
			key_card_entity.number[0] = key_card_entity.number[0] | 0x80000000;
			key_card_entity.one_number++;
			key_card_entity.odd_one_number++;

		} else {

			wiegand_entity.number = wiegand_entity.number << 1;
			wiegand_entity.number = wiegand_entity.number | 0x01;   

			key_card_entity.number[0] = key_card_entity.number[0] >> 1;
			key_card_entity.one_number = 0;
		}
	} 

	if (key_card_entity.one_number == 1) {
		/*if strip the code, it will be wiegand format support*/
		key_card_entity.start++;
	} 

	if (key_card_entity.start == 1) {

		key_card_entity.odd_start = 1;
		/*interrupt_times =11*/
		key_card_entity.interrupt_flags = key_card_entity.interrupt_times - 1;

		/*key_card_entity.flags_card = 1; */
		key_card_entity.start = -100;   
	}

	if ((key_card_entity.interrupt_times >
		(32+key_card_entity.interrupt_flags)) &&
		(key_card_entity.interrupt_times <
		(65+key_card_entity.interrupt_flags))) {
		if ( !s3c2410_gpio_getpin(data)) {
			key_card_entity.number[1] = key_card_entity.number[1] >> 1;
			key_card_entity.number[1] = key_card_entity.number[1] | 0x80000000;
			key_card_entity.one_number++;
			key_card_entity.odd_one_number++;    
		} else {
			key_card_entity.number[1] = key_card_entity.number[1] >> 1;
			key_card_entity.one_number = 0;
		}   
	} 
	if (key_card_entity.interrupt_times >
		(64+key_card_entity.interrupt_flags)) {
		if ( !s3c2410_gpio_getpin(data)) {
			key_card_entity.number[2] = key_card_entity.number[2] >> 1;
			key_card_entity.number[2] = key_card_entity.number[2] | 0x80000000;
			key_card_entity.one_number++;
			key_card_entity.odd_one_number++;       
		} else {
			key_card_entity.number[2] = key_card_entity.number[2] >> 1;
			key_card_entity.one_number = 0;    
		}                
	}

	if (key_card_entity.odd_start) {
		key_card_entity.verify_number++; 
		if (!(key_card_entity.verify_number % 5)) {
			if(key_card_entity.odd_one_number % 2) {
				/*printk("odd verify success\n");*/
			} else {
				memset(&key_card_entity,0,init_entity);
				printk("error\n");
				return 0;
			}
			key_card_entity.odd_one_number = 0;
		}
	}

	if (key_card_entity.one_number == 5) {
		key_card_entity.flags_after = 1;
	}

	if (key_card_entity.flags_after == 1) {
		key_card_entity.times_after++;

		if (key_card_entity.times_after == 6) {
			kill_fasync(&key_card_entity.async_queue, SIGIO, POLL_IN);
		}
	}

	local_irq_enable();
	return 0;
}

static int key_fasync(int fd, struct file *filp, int mode)
{
	printk("touch fasync\n");
	return fasync_helper(fd,filp,mode,&key_card_entity.async_queue);
}

static int key_open(struct inode *inode,struct file *filp)
{
	int irq_err;

	irq_err = request_irq(key_irqs[0].irq, clock_interrupt, IRQF_TRIGGER_FALLING, 
			key_irqs[0].name, (void *)&key_irqs[0].id);
	if (irq_err) {
		printk("request_irq error %d",key_irqs[0].id);
	}  
	irq_err = request_irq(key_irqs[1].irq, data_interrupt, IRQF_TRIGGER_FALLING, 
			key_irqs[1].name, (void *)&key_irqs[1].id);
	if (irq_err) {
		printk("request_irq error %d",key_irqs[1].id);
	}  
	printk("request_irq success\n");

	init_timer(&key_card_entity.card_timer);
	key_card_entity.card_timer.function = card_timer_func;
	add_timer(&key_card_entity.card_timer);

	spin_lock_init(&key_card_entity.card_lock);
	return 0;
}
static int key_close(struct inode *inode, struct file *file)
{
	int i;

	for (i = 0; i < sizeof(key_irqs)/sizeof(key_irqs[0]); i++) {
		free_irq(key_irqs[i].irq, (void *)&key_irqs[i].id);
	}
	key_fasync(-1,file,0);
	del_timer(&key_card_entity.card_timer); 
	return 0;
}

static int key_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	unsigned long err = 0;
	unsigned int i;

	unsigned int card_number[key_card_entity.flags_card_end];
	unsigned int j;

	disable_irq_nosync(key_irqs[0].irq);

	printk("key_read interrupt_times is  :%d\n", key_card_entity.interrupt_times);

	key_card_entity.flags_card_end = (key_card_entity.interrupt_times -
					key_card_entity.interrupt_flags - 15)/5;

	if (key_card_entity.flags_after) { 
		if (!((key_card_entity.number[0] & 0xf) == 0xb)) {
			printk("error\n");
			printk("key_card_entity.number[0] :%x\n",
				key_card_entity.number[0]);
			memset(&key_card_entity,0,init_entity);
			return 0;     
		} else {
			printk("the front of 15 number verify success\n");
		}

		unsigned int l = 0;
		unsigned int num_copy  = key_card_entity.number[0];
		unsigned char LRC_char[4] = {0, 0, 0, 0};
		for (l = 1; l <= key_card_entity.interrupt_times; l++) {
			if ((l %5) == 1) {
				if ( num_copy & 0x1 )         
					LRC_char[0]++; 
				num_copy >>=1;    
			}
			if ((l %5) == 2) {
				if ( num_copy & 0x1 )
					LRC_char[1]++;
				num_copy >>=1;    
			}
			if ((l %5) == 3) {
				if ( num_copy & 0x1 )
					LRC_char[2]++;
				num_copy >>=1;    
			}
			if ((l %5) == 4) {
				if ( num_copy & 0x1 )
					LRC_char[3]++;
				num_copy >>=1;    
			}

			if ((l %5) == 0) {
				num_copy >>=1;    
			}
			if ( l == 30) {
				num_copy = (key_card_entity.number[0] >> 30) |
				(key_card_entity.number[1] << 2);    
			}
			if ( l == 60) {
				num_copy = (key_card_entity.number[1] >> 28) |
				(key_card_entity.number[2] >> (32 -
				((key_card_entity.interrupt_times -
				key_card_entity.interrupt_flags) - 64) - 4));
			}
		}

		for (l = 0; l<4; l++) {
			if (!(LRC_char[l] % 2)) {
				printk("LRC_char[%d] is success\n",l);
			} else {
				printk("LRC_char[%d] is error\n",l);
				memset(&key_card_entity,0,init_entity);
			}  
		}

		err=copy_to_user(buff,key_card_entity.number, 3*sizeof (int));  
		for (i = 0 ; i < 3; i++) {
			printk("key_card_entity.number[%d] is: %x\n",
			i ,key_card_entity.number[i]);
		}   

		printk("card_number is: ");
		key_card_entity.number[0] = key_card_entity.number[0] >> 5;

		for (j = 1; j <= key_card_entity.flags_card_end;j++) {          
			if (j < 6) {
				card_number[j] = key_card_entity.number[0] & 0xf;
				key_card_entity.number[0] >>= 5; 
			}
			if ( j == 6) {
				card_number[j] = key_card_entity.number[0] & 0x3;
				card_number[j] |= key_card_entity.number[1] & 0x3;
				key_card_entity.number[1] >>= 3;
			}
			if( j>6 && j<13) {
				card_number[j] = key_card_entity.number[1] & 0xf;
				key_card_entity.number[1] >>= 5; 
			}
			if ( j == 12 )
				key_card_entity.number[2]>>=(32 -
				((key_card_entity.interrupt_times -
				key_card_entity.interrupt_flags) - 64) +1 );
			if (j > 12) {
				card_number[j] = key_card_entity.number[2] & 0xf;
				key_card_entity.number[2] >>= 5; 

			}
			printk("%x",card_number[j]);
		}
		printk("\ncard_number is success\n");
	} else if (((key_card_entity.interrupt_times +
		key_card_entity.data_interrupt_times) == 4) ||
		((key_card_entity.interrupt_times +
		key_card_entity.data_interrupt_times)== 26)) {
		key_card_entity.flags = wiegand_entity.number;  
		err = copy_to_user(buff,&key_card_entity.flags, sizeof (int)); 
	} else {
		/*key number*/
		err = copy_to_user(buff,&key_card_entity.flags, sizeof (int));
	}

	if (err) {
		printk("copy_to_user is fail\n"); 
		return 0;
	}

	printk("copy_to_user is success\n");

	memset(&key_card_entity,0,init_entity);

	memset(&wiegand_entity, 0, sizeof(wiegand_entity));

	enable_irq(key_irqs[0].irq);

	return err ? -EFAULT : 0;
}

static struct file_operations key_fops = {
	.owner    = THIS_MODULE,
	.open     = key_open,
	.release  = key_close,
	.read     = key_read,
	.fasync   = key_fasync,
};

static struct miscdevice misc = {
	.minor = KEY_MINOR,
	.name = DEVICE_NAME,
	.fops = &key_fops,
};

static int __init key_read_init(void)
{
	int ret;

	ret = misc_register(&misc);

	if (ret < 0) {
		printk(DEVICE_NAME " can't register major number[0]\n");
		return ret;
	}

	printk(DEVICE_NAME " initialized\n");

	return 0;
}

static void __exit key_read_exit(void)
{
	misc_deregister(&misc);
}

module_init(key_read_init);
module_exit(key_read_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("key device drver");
MODULE_AUTHOR("Jim");
