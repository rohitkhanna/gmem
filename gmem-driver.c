/* My driver  for Lab 1*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>


#define DEVICE_NAME  "gmem"			// device would be named as /dev/gmem
#define CB_SIZE 256					// define our circular buffer size used in initCB()


typedef struct cb{				/* Circular Buffer struct to be used for reading and writing */
	char *elems;				// elements
	int size;					// size of circular buffer
	int tail;					// points to the first empty location in the buffer or to the '\0' char of the last written string
								// tail will move from  (0 - size-1) only
}CB;

/* per device structure */
struct My_dev {
	struct cdev cdev;               	 /* The cdev structure */
	char name[20];                 		 /* Name of device*/
	CB *cb;								 /* Circular Buffer */
} *my_devp;

static dev_t my_dev_number;      		/* Allotted device number */
struct class *my_dev_class;          		/* Tie with the device model */

/* Function Declarations */
void initCB(CB **cb);
char* readCB(CB *cb);
void writeToCB(CB **cb, char ch);
void writeStringToCB(CB **cb, char *str);

/*
 * Open My driver
 */
int My_driver_open(struct inode *inode, struct file *file)
{
	struct My_dev *my_devp;

	printk("\nMy_driver_open()\n");

	/* Get the per-device structure that contains this cdev */
	my_devp = container_of(inode->i_cdev, struct My_dev, cdev);

	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = my_devp;

	printk("%s has opened\n", my_devp->name);
	return 0;
}


/*
 * Release My driver
 */
int My_driver_release(struct inode *inode, struct file *file)
{
	struct My_dev *my_devp = file->private_data;

	printk("\n%s is closing\n", my_devp->name);

	return 0;
}


/*
 * Write to my driver
 */
ssize_t My_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int res;
	char str[CB_SIZE];
	struct My_dev *my_devp = file->private_data;
	printk("My_driver_write()\n");

	res = copy_from_user((void *)&str, (void __user *)buf, count);
	if(res){
		return -EFAULT;
	}

	printk("\ndata from User = |%s| ,res = %d, count=%d\n", str, res, count);

	writeStringToCB(&(my_devp->cb), str);							// Write str to our CB
	readCB(my_devp->cb);											// Read CB for debugging

	return res;
}


/*
 * Read from my driver
 */
static ssize_t My_driver_read(struct file *file, char *buf, size_t count, loff_t *ptr){

	int len;
	struct My_dev *my_devp = file->private_data;
	printk("My_driver_read()\n");

	len =  strlen((my_devp->cb)->elems);
	printk("cb = \n||%s||\n with strlen=%d and tail=%d\n",(my_devp->cb)->elems, len,(my_devp->cb)->tail	 );

	if (  copy_to_user(buf, (my_devp->cb)->elems, len+1)  ) {		//len + 1 because of the '\0' char
		return -EFAULT;
	}

	return 0;
}



/* File operations structure. Defined in linux/fs.h */
static struct file_operations My_fops = {
		.owner = THIS_MODULE,           /* Owner */
		.open = My_driver_open,              /* Open method */
		.release = My_driver_release,        /* Release method */
		.write = My_driver_write,            /* Write method */
		.read = My_driver_read		/* Read method */
};


/*
 * Driver Initialization
 */
int __init My_driver_init(void)
{
	int ret;
	printk("My_driver_init()\n");

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&my_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	printk("My major number = %d, Minor number = %d\n", MAJOR(my_dev_number), MINOR(my_dev_number));

	/* Populate sysfs entries */
	my_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	my_devp = kmalloc(sizeof(struct My_dev), GFP_KERNEL);
	if (!my_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(my_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&my_devp->cdev, &My_fops);
	my_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&my_devp->cdev, (my_dev_number), 1);
	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 0), NULL, DEVICE_NAME);	


	/* Init the 256 byte circular buffer  */
	initCB( &(my_devp->cb) );

	printk("init() - The address of %ld is %p\n", jiffies, &jiffies);		// To find the address of jiffies variable

	printk("My Driver = %s Initialized.\n", my_devp->name);
	return 0;
}


/*
 * Used to initialize our Circular Buffer cb
 */
void initCB(CB **cb){

	int i;
	char init_string[100];								// Hold the initial string
	printk("initCB()\n");

	(*cb) = kmalloc(sizeof(CB), GFP_KERNEL);			//Allocate memory to cb struct
	if(!*cb){
		printk("Couldnt allocated mem to cb\n"); return;
	}
	(*cb)->size = CB_SIZE;
	(*cb)->elems = kmalloc(sizeof(char) * (*cb)->size, GFP_KERNEL);

	for(i=0; i<CB_SIZE; i++)							//Init the circular buffer with blank characters
		(*cb)->elems[i] = ' ';

	(*cb)->tail = 0;									// tail  points to 0 as there is no data initially

	sprintf(init_string, "Hello world! This is Rohit Khanna, and this machine has worked for %ld seconds.", jiffies/HZ);
	strcpy((*cb)->elems, init_string);					// copy init string to our CB
	readCB(*cb);

	(*cb)->tail += strlen(init_string);						//Tail points to '\0' char
	printk("tail=%d \n", (*cb)->tail);

	printk("CB Initialized\n");

}

/*
 * Read and print the contents of CB
 */

char* readCB(CB *cb){

	printk("readCB()\n");
	if(!cb){
		printk("cb is NULL\n");
		return NULL;
	}
	printk("CB = \n<<<%s>>>\n", cb->elems);
	return (cb)->elems;

}


/*
 * Write a string to the Circular Buffer -
 * -- writes char by char 
 */
void writeStringToCB(CB **cb, char *str){

	int i;
	printk("writeStringToCB()\n");
	if(!str){
		printk("str is NULL\n");
		return;
	}

	if(!cb || !*cb){
		printk("cb is NULL\n");
		return;
	}

	printk("writing str=<<%s>> with strlen=%d to (cb)=\n{{%s}} \n with Tail = %d\n", str, strlen(str), (*cb)->elems,(*cb)->tail);
	for(i=0; i<=strlen(str); i++){							// Will also write the '\0' char to the buffer
		writeToCB(cb, str[i]);
	}
	printk("Tail after writing is %d\n", (*cb)->tail);		//Debugging


}



/*
 * Write a char to the Circular Buffer
 * -- wraps around if finds end of buffer
 */
void writeToCB(CB **cb, char ch){

	//printk("Writing %c to tail=%d", ch,(*cb)->tail);
	if(!cb || !*cb){
		printk("cb is NULL\n");
		return;
	}

	(*cb)->elems[(*cb)->tail] = ch;										// write char and then increment
	(*cb)->tail = ( ((*cb)->tail + 1)  %  ((*cb)->size - 1) );			// rolls over when reaches (cb->size -1), tail moves from (0 - size-1) only
	if(ch == '\0')
		(*cb)->tail = (*cb)->tail - 1;									//want the tail to point to the '\0' char
	//printk("%c", (*cb)->elems[(*cb)->tail]);
}




/*
 * Driver Exit
 */
void __exit My_driver_exit(void)
{

	/* Release the major number */
	unregister_chrdev_region((my_dev_number), 1);

	/* Destroy device */
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 0));
	cdev_del(&my_devp->cdev);
	kfree(my_devp);

	/* Destroy driver_class */
	class_destroy(my_dev_class);

	printk("My Driver exits.\n");
}

module_init(My_driver_init);
module_exit(My_driver_exit);
MODULE_LICENSE("GPL v2");
