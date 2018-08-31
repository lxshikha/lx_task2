#include <linux/i2c.h>
#include <linux/delay.h>

#include "lis3dh_acc.h"

int lis3dh_acc_i2c_read(struct i2c_client *client, u8 *buffer, size_t length)
{
    struct i2c_msg packets[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = buffer,
         },
         {
             .addr = client->addr,
             .flags = I2C_M_RD,
             .len = length,
             .buf = buffer,
        }
    };
    
    return i2c_transfer(client->adapter, packets, 2);
}

static int lis3dh_acc_i2c_write(struct i2c_client *client, u8 *buffer, size_t length)
{
    struct i2c_msg packets[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = length,
            .buf = buffer,
         }
    };
    return i2c_transfer(client->adapter, packets, 1);
}

/* Return 1 if device sucessfully identified, 0 otherwise */
int lis3dh_acc_identify(struct i2c_client *client)
{
    u8 buffer[] = {WHO_AM_I};
    int status = lis3dh_acc_i2c_read(client, buffer, 1);
    printk("Device identification (read: 0x%x, expected 0x%x)\n", buffer[0], WHO_AM_I_OUTPUT);
    return ((status == 2) && (buffer[0] == WHO_AM_I_OUTPUT));
}

int lis3dh_acc_power_on(struct i2c_client *client)
{
    int status;
    u8 i2c_power_on[] = {
        CTRL_REG1,
        POWER_ON_50_HZ
    };
    status = lis3dh_acc_i2c_write(client, i2c_power_on, sizeof(i2c_power_on) / sizeof(u8));
    printk("sh pwr =%d\n", status);
    return (status >= 0);
}

int lis3dh_acc_power_off(struct i2c_client *client)
{
    int status;
    u8 i2c_power_off[] = {
        CTRL_REG1,
        POWER_OFF
    };
    status = lis3dh_acc_i2c_write(client, i2c_power_off, sizeof(i2c_power_off) / sizeof(u8));
    return (status >= 0);
}

int lis3dh_acc_int1_enable(struct i2c_client *client)
{
    int status;
    u8 i2c_data[2];

    i2c_data[0] = INT1_CFG;
    i2c_data[1] = CONFIG_INT1_ENABLE;
    status = lis3dh_acc_i2c_write(client, i2c_data, sizeof(i2c_data) / sizeof(u8));

    if (status >= 0) {
        i2c_data[0] = INT1_THS;
        i2c_data[1] = CONFIG_INT1_THS;
        status = lis3dh_acc_i2c_write(client, i2c_data, sizeof(i2c_data) / sizeof(u8));
    }

    if (status >= 0) {
        i2c_data[0] = INT1_DURATION;
        i2c_data[1] = CONFIG_INT1_DURATION;
        status = lis3dh_acc_i2c_write(client, i2c_data, sizeof(i2c_data) / sizeof(u8));
    }

    if (status >= 0) {
        i2c_data[0] = CTRL_REG3;
        i2c_data[1] = CONFIG_CTRL_REG3;
        status = lis3dh_acc_i2c_write(client, i2c_data, sizeof(i2c_data) / sizeof(u8));
    }

    return (status >= 0);
}

int lis3dh_acc_int1_set_threshold(struct i2c_client *client, int threshold)
{
    int status;
    u8 i2c_data[] = {
	INT1_THS,
        threshold
    };
    status = lis3dh_acc_i2c_write(client, i2c_data, sizeof(i2c_data) / sizeof(u8));
    return (status >= 0);
}

/* We expect axis of length 3 (X,Y,Z) */
/* Checking status register first - Then read 6 acceleration bytes */
int lis3dh_acc_get_acceleration(struct i2c_client *client, s16 *axis)
{
    int status;
    u8 i2c_data[6] = { STATUS_REG, 0, 0, 0, 0, 0};
    status = lis3dh_acc_i2c_read(client, i2c_data, 1);
    if (!i2c_data[0]) {
	/* Acceleration value has not changed, just wait */
        msleep_interruptible(CONFIG_RETRY_MS);
    }
    if (axis) {
        /* Get fresh acceleration value */
        i2c_data[0] = (I2C_AUTO_INCREMENT | OUT_X_L);
        status = lis3dh_acc_i2c_read(client, i2c_data, 6);
        if (status >= 0) {
            axis[0] = (((s16) ((i2c_data[1] << 8) | i2c_data[0])) >> 4);
            axis[1] = (((s16) ((i2c_data[3] << 8) | i2c_data[2])) >> 4);
            axis[2] = (((s16) ((i2c_data[5] << 8) | i2c_data[4])) >> 4);
            //printk("Acceleration: %d, %d, %d (ms)\n", axis[0], axis[1], axis[2]);
	    //return (status >= 0); 
        }
    }
    return (status >= 0);
}
