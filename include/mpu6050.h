#ifndef MPU6050_H_
#define MPU6050_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

#define MPU6050_RESET 0x80
#define MPU6050_ID 0x68
#define MPU6050_ACCEL_LSB_PER_G 16384
#define MPU6050_GYRO_LSB_PER_DPS 131

struct mpu6050_data {
  int16_t accel[3];
  int16_t gyro[3];
};

struct mpu6050_config {
  struct i2c_dt_spec i2c;
};

#endif
