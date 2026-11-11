#ifndef MPU6050_H_
#define MPU6050_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define MPU6050_REG_SMPLRT_DIV 0x19
#define MPU6050_REG_CONFIG 0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

// FS_SEL and AFS_SEL are bits [4:3] of their registers
#define MPU6050_FS_SHIFT 3

#define MPU6050_RESET 0x80
#define MPU6050_ID 0x68

struct mpu6050_data {
  int16_t accel[3];
  int16_t gyro[3];
  int32_t accel_lsb_per_g;
  int32_t gyro_lsb_per_dps_x10;
};

struct mpu6050_config {
  struct i2c_dt_spec i2c;
  uint16_t gyro_fs;
  uint8_t accel_fs;
  uint8_t smplrt_div;
  uint8_t dlpf_cfg;
};

#endif
