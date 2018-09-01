#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "lis3dh_acc.h"

#define LX_ACCELL_DRIVER_DEV_NAME "lxaccell"
#define LX_ACCELL_GPIO_INT1 49

struct lx_accell_private_data {
    struct i2c_client *client;
    s16 acc[3];
    int irq;
    struct fasync_struct *async_queue;
};

/* Utilities*/
static void misc_set_drvdata(struct miscdevice *misc, void *pdata)
{
    if (misc && misc->this_device) {
        dev_set_drvdata(misc->this_device, pdata);
    }
};

static void *misc_get_drvdata(struct file *file)
{
    struct miscdevice *misc = file->private_data;
    if (misc && misc->this_device) {
        return dev_get_drvdata(misc->this_device);
    }
    return NULL;
}

irqreturn_t lx_accell_interrupt(int irq, void *context)
{
    struct lx_accell_private_data *pdata = context;
    printk(">>>>>> Inside interupt handler <<<<<<<\n");

    if (pdata->async_queue) {
       kill_fasync(&pdata->async_queue, SIGIO, POLL_IN);
    }
    return IRQ_HANDLED;
}

static int lx_accell_install_irq(struct lx_accell_private_data *pdata)
{
    int status;
    printk("GPIO %d is valid? %s\n", LX_ACCELL_GPIO_INT1, gpio_is_valid(LX_ACCELL_GPIO_INT1) ? "yes" : "no");
    gpio_request(LX_ACCELL_GPIO_INT1, "sysfs");
    gpio_direction_input(LX_ACCELL_GPIO_INT1);
    gpio_export(LX_ACCELL_GPIO_INT1, false);

    pdata->irq = gpio_to_irq(LX_ACCELL_GPIO_INT1);
    printk("GPIO %d mapped to IRQ %d\n", LX_ACCELL_GPIO_INT1, pdata->irq);

    status = request_irq(pdata->irq, lx_accell_interrupt, IRQF_TRIGGER_RISING, "lx_accell", pdata);
    if (status) {
        printk("Failed to install IRQ handler (status=%d)\n", status);
    }

    lis3dh_acc_int1_enable(pdata->client);
    return (status >= 0);
}

static void lx_accell_remove_irq(struct lx_accell_private_data *pdata)
{
    free_irq(pdata->irq, NULL);
    gpio_unexport(LX_ACCELL_GPIO_INT1);
    gpio_free(LX_ACCELL_GPIO_INT1);
}

static int lx_accell_device_fasync(int fd, struct file *file, int mode)
{
    struct lx_accell_private_data *pdata = misc_get_drvdata(file);
    printk("Inside async\n");
    return fasync_helper(fd, file, mode, &pdata->async_queue);
}

static int lx_accell_device_open(struct inode *inode, struct file *file)
{
    struct lx_accell_private_data *pdata = misc_get_drvdata(file);
    printk("Reading device pdata=%p\n", pdata);
    return 0;
}

static int lx_accell_device_release(struct inode *inode, struct file *file)
{
    printk("Closing device\n");
    /* removing this file from async queue */
    lx_accell_device_fasync(-1, file, 0);
    return 0;
}

static ssize_t lx_accell_device_read(struct file *file, char __user *buffer,
                                                size_t length, loff_t *offset)
{
    int status = 0;
    u8 i2c_data[6] = { STATUS_REG, 0, 0, 0, 0, 0};

    struct lx_accell_private_data *pdata = misc_get_drvdata(file);

    /* Below offset check is to stop "cat" to read accellerometer data recursively */ 
    if(*offset > 0)
        return 0;

    if (pdata && pdata->client && pdata->client->adapter) {
	 status = lis3dh_acc_i2c_read(pdata->client, i2c_data, 1);
    	 if (!i2c_data[0]) {
        	/* Acceleration value has not changed, just wait */
        	msleep_interruptible(CONFIG_RETRY_MS);
         }	
	 if (pdata->acc) {
		/* Get fresh acceleration value */
        	i2c_data[0] = (I2C_AUTO_INCREMENT | OUT_X_L);
        	status = lis3dh_acc_i2c_read(pdata->client, i2c_data, 6);
       		if (status >= 0) {
            		pdata->acc[0] = (((s16) ((i2c_data[1] << 8) | i2c_data[0])) >> 4);
            		pdata->acc[1] = (((s16) ((i2c_data[3] << 8) | i2c_data[2])) >> 4);
            		pdata->acc[2] = (((s16) ((i2c_data[5] << 8) | i2c_data[4])) >> 4);
		}
	}
	length = snprintf(buffer, length, "%d,%d,%d\n", pdata->acc[0], pdata->acc[1], pdata->acc[2]);
    }

    *offset = *offset + length;

    return (status ? length : -EIO);
}

static ssize_t lx_accell_device_write(struct file *file, const char __user *buffer,
		       size_t length, loff_t *offset)
{
    struct lx_accell_private_data *pdata = misc_get_drvdata(file);
    unsigned long threshold = 0;
    if (strstr(buffer, "db,") && !kstrtoul(buffer+3, 0, &threshold)) {
        printk("Applying new threshold: %lu\n", threshold);
        lis3dh_acc_int1_set_threshold(pdata->client, threshold);
    }
    return length;
}

static const struct file_operations lx_accell_device_operations = {
    .owner              = THIS_MODULE,
    .read               = lx_accell_device_read,
    .write              = lx_accell_device_write,
    .open               = lx_accell_device_open,
    .release            = lx_accell_device_release,
    .fasync             = lx_accell_device_fasync
};

struct miscdevice lx_accell_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = LX_ACCELL_DRIVER_DEV_NAME,
    .fops = &lx_accell_device_operations,
    .parent = NULL,
    .this_device = NULL
};

static int lx_accell_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
    struct lx_accell_private_data *pdata;
    int error;

    pdata = kzalloc(sizeof(struct lx_accell_private_data), GFP_KERNEL);
    if (pdata) {
	pdata->client = client;
        i2c_set_clientdata(client, pdata);
    }

    error = misc_register(&lx_accell_device);
    if (error) {
        printk("Failed to register device %s\n", LX_ACCELL_DRIVER_DEV_NAME);
        return error;
    }

    misc_set_drvdata(&lx_accell_device, pdata);

    /* Wake up the accelerometer */
    lis3dh_acc_power_on(client);

    /* Setup IRQ for motion detection */
    lx_accell_install_irq(pdata);

    return 0;
}

static int lx_accell_i2c_remove(struct i2c_client *client)
{
    void *pdata = i2c_get_clientdata(client);

    lis3dh_acc_power_off(client);
    lx_accell_remove_irq(pdata);
    misc_deregister(&lx_accell_device);

    printk("Inside i2c_remove\n");
    if (pdata)
        kfree(pdata);

    return 0;
}

static int lx_accell_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    printk("Inside i2c_detect i2c_board_info: addr=%x, adapter=%p\n", info->addr, client->adapter);

    if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C)
	    && lis3dh_acc_identify(client)) {
	strcpy(info->type, LX_ACCELL_DRIVER_DEV_NAME);
    }

    return 0;
}

static const struct i2c_device_id lx_accell_i2c_driver_id[] = {
    { LX_ACCELL_DRIVER_DEV_NAME, 0},
    { }
};

MODULE_DEVICE_TABLE(i2c, lx_accell_i2c_driver_id);

const unsigned short lx_accell_i2c_address_list[] = {
    I2C_ADDR_PRIMARY,
    I2C_ADDR_SECONDARY
};

static struct i2c_driver lx_accell_i2c_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = LX_ACCELL_DRIVER_DEV_NAME,
    },
    .id_table = lx_accell_i2c_driver_id,
    .probe = lx_accell_i2c_probe,
    .remove = lx_accell_i2c_remove,

    /* For auto-detect*/
    .class		= I2C_CLASS_HWMON,
    .detect		= lx_accell_i2c_detect,
    .address_list	= lx_accell_i2c_address_list
};

static int __init lx_driver_init(void)
{
    int error;

    printk("LX Driver init\n");

    error = i2c_add_driver(&lx_accell_i2c_driver);
    if (error) {
        printk("Failed to register i2c driver for device %s\n", LX_ACCELL_DRIVER_DEV_NAME);
        return error;
    }

    printk("Successful register device %s\n", LX_ACCELL_DRIVER_DEV_NAME);
    return 0;
}

static void __exit lx_driver_exit(void)
{
    i2c_del_driver(&lx_accell_i2c_driver);
    printk("LX Driver exit\n");
}

module_init(lx_driver_init)
module_exit(lx_driver_exit)

MODULE_DESCRIPTION("Misc driver for LIS3DH accelerometer");
MODULE_LICENSE("GPL");
