#ifndef _LIS3DH_ACC_H_
#define _LIS3DH_ACC_H_

#define I2C_ADDR_PRIMARY     (0x18)
#define I2C_ADDR_SECONDARY   (0x19)
#define I2C_AUTO_INCREMENT   (0x80) /* MSB on 8 bits address */
#define WHO_AM_I_OUTPUT      (0x33)
#define POWER_ON_50_HZ       (0x47) /* 0b01000111
                                       > 0100----: (ODR) Normal / low power mode (50 Hz)
                                       > ----1---: Low power mode enabled
                                       > -----111: XYZ axis enabled */
#define POWER_OFF            (0x00)

#define CONFIG_RETRY_MS      (40)   /* 40 ms (slightly about 50Hz period) */

#define CONFIG_CTRL_REG3     (0x40) /* 0b01000000: AOI1 interrupt on INT1 */
#define CONFIG_INT1_ENABLE   (0x3f) /* 0b00111111
                                       > 00------: OR Combination of interruots events
                                       > --111111: XYZ Low / hight enabled */
#define CONFIG_INT1_THS      (0x02) /* Arbitrary */
#define CONFIG_INT1_DURATION (0x40) /* Arbitrary */

/* LIS3DH registers */
enum lis3dh_registers {
    STATUS_REG_AUX = 0x07,
    OUT_ADC1_L,
    OUT_ADC1_H,
    OUT_ADC2_L,
    OUT_ADC2_H,
    OUT_ADC3_L,
    OUT_ADC3_H,
    INT_COUNTER_REG,
    WHO_AM_I,
    TEMP_CFG_REG = 0x1f,
    CTRL_REG1,
    CTRL_REG2,
    CTRL_REG3,
    CTRL_REG4,
    CTRL_REG5,
    CTRL_REG6,
    REFERENCE,
    STATUS_REG,
    OUT_X_L,
    OUT_X_H,
    OUT_Y_L,
    OUT_Y_H,
    OUT_Z_L,
    OUT_Z_H,
    FIFO_CTRL_REG,
    FIFO_SRC_REG,
    INT1_CFG,
    INT1_SOURCE,
    INT1_THS,
    INT1_DURATION,
    CLICK_CFG = 0x38,
    CLICK_SRC,
    CLICK_THS,
    TIME_LIMIT,
    TIME_LATENCY,
    TIME_WINDOW
};

int lis3dh_acc_identify(struct i2c_client *client);
int lis3dh_acc_power_on(struct i2c_client *client);
int lis3dh_acc_power_off(struct i2c_client *client);
int lis3dh_acc_int1_enable(struct i2c_client *client);
int lis3dh_acc_int1_set_threshold(struct i2c_client *client, int threshold);
int lis3dh_acc_get_acceleration(struct i2c_client *client, s16 *axis);
int lis3dh_acc_i2c_read(struct i2c_client *client, u8 *buffer, size_t length);
#endif /*_LIS3DH_ACC_H_ */
