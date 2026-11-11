#define DT_DRV_COMPAT mpu6050

#include "mpu6050.h"
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mpu6050, CONFIG_SENSOR_LOG_LEVEL);

static int mpu6050_sample_fetch(const struct device *dev,
                                enum sensor_channel chan) {
  const struct mpu6050_config *config = dev->config;
  struct mpu6050_data *data = dev->data;
  uint8_t registers[14];
  int err;
  // Check if the requested sensor channel is supported
  if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_XYZ &&
      chan != SENSOR_CHAN_GYRO_XYZ) {
    LOG_ERR("Unsupported sensor channel");
    return -ENOTSUP;
  }
  // Burst read accelerometer, temperature and gyroscope data from the sensor
  // (as recommended by the datasheet)
  err = i2c_burst_read_dt(&config->i2c, MPU6050_REG_ACCEL_XOUT_H, registers,
                          sizeof(registers));
  if (err < 0) {
    LOG_ERR("Failed to read sensor data");
    return err;
  }
  // Convert 8-bit register data to 16-bit signed integers
  for (int i = 0; i < 3; i++) {
    data->accel[i] = registers[2 * i] << 8 | registers[2 * i + 1];
    data->gyro[i] = registers[2 * i + 8] << 8 | registers[2 * i + 9];
  }
  return 0;
}

static int mpu6050_channel_get(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val) {
  const struct mpu6050_data *data = dev->data;
  const int16_t *raw;
  int64_t numerator;
  int64_t denominator;
  // Select the appropriate raw data and conversion factors based on the
  // requested sensor channel
  switch (chan) {
  case SENSOR_CHAN_ACCEL_XYZ:
    raw = data->accel;
    numerator = SENSOR_G;
    denominator = data->accel_lsb_per_g;
    break;
  case SENSOR_CHAN_GYRO_XYZ:
    raw = data->gyro;
    numerator = (int64_t)SENSOR_PI * 10;
    denominator = (int64_t)data->gyro_lsb_per_dps_x10 * 180;
    break;
  default:
    return -ENOTSUP;
  }
  // Convert raw sensor data to sensor_value format for each axis
  for (int i = 0; i < 3; i++) {
    int err =
        sensor_value_from_micro(&val[i], raw[i] * numerator / denominator);
    if (err < 0) {
      LOG_ERR("Failed to convert sensor value");
      return err;
    }
  }
  return 0;
}

static DEVICE_API(sensor, mpu6050_api) = {
    .sample_fetch = &mpu6050_sample_fetch,
    .channel_get = &mpu6050_channel_get,
};

static int mpu6050_who_am_i(const struct device *dev) {
  const struct mpu6050_config *config = dev->config;
  uint8_t identity;
  int err;
  // Read WHO_AM_I register
  err = i2c_reg_read_byte_dt(&config->i2c, MPU6050_REG_WHO_AM_I, &identity);
  if (err < 0) {
    LOG_ERR("Failed to read device ID");
    return err;
  }
  // Verify the identity of the device
  if (identity != MPU6050_ID) {
    LOG_ERR("unexpected device ID 0x%02x", identity);
    return -ENODEV;
  }
  return 0;
}

static const uint16_t gyro_lsb_per_dps_x10[] = {1310, 655, 328, 164};

static int mpu6050_configure(const struct device *dev) {
  const struct mpu6050_config *config = dev->config;
  struct mpu6050_data *data = dev->data;
  uint8_t fs_sel;
  uint8_t afs_sel;
  int err;

  // Map the devicetree range to the register field: 250 << FS_SEL deg/s
  // 250 500 1000 2000
  for (fs_sel = 0; fs_sel < 4; fs_sel++) {
    if ((250U << fs_sel) == config->gyro_fs) {
      break;
    }
  }
  // 2 << AFS_SEL g
  // 2 4 8 16
  for (afs_sel = 0; afs_sel < 4; afs_sel++) {
    if ((2U << afs_sel) == config->accel_fs) {
      break;
    }
  }
  if (fs_sel == 4 || afs_sel == 4) {
    LOG_ERR("Invalid full-scale range");
    return -EINVAL;
  }

  err =
      i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_CONFIG, config->dlpf_cfg);
  if (err < 0) {
    LOG_ERR("Failed to write CONFIG");
    return err;
  }
  err = i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_SMPLRT_DIV,
                              config->smplrt_div);
  if (err < 0) {
    LOG_ERR("Failed to write SMPLRT_DIV");
    return err;
  }
  err = i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_GYRO_CONFIG,
                              fs_sel << MPU6050_FS_SHIFT);
  if (err < 0) {
    LOG_ERR("Failed to write GYRO_CONFIG");
    return err;
  }
  err = i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_ACCEL_CONFIG,
                              afs_sel << MPU6050_FS_SHIFT);
  if (err < 0) {
    LOG_ERR("Failed to write ACCEL_CONFIG");
    return err;
  }

  data->gyro_lsb_per_dps_x10 = gyro_lsb_per_dps_x10[fs_sel];
  data->accel_lsb_per_g = 16384 >> afs_sel;
  return 0;
}

static int mpu6050_init(const struct device *dev) {
  const struct mpu6050_config *config = dev->config;
  int err;
  // Check if i2c bus is ready before proceeding
  if (!i2c_is_ready_dt(&config->i2c)) {
    LOG_ERR("I2C not ready");
    return -ENODEV;
  }
  // Fetch and verify device ID to check if the device is present and correct
  err = mpu6050_who_am_i(dev);
  if (err < 0) {
    LOG_ERR("Failed to read device ID");
    return err;
  }
  // Reset the device to ensure it starts in default state
  err = i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_PWR_MGMT_1,
                              MPU6050_RESET);
  if (err < 0) {
    LOG_ERR("Failed to reset device");
    return err;
  }
  k_msleep(100);

  // Wake up device and use one of the gyroscopes as clock source (recommended
  // by datasheet)
  err = i2c_reg_write_byte_dt(&config->i2c, MPU6050_REG_PWR_MGMT_1, 0x01);
  if (err < 0) {
    LOG_ERR("Failed to wake up device");
    return err;
  }

  err = mpu6050_configure(dev);
  if (err < 0) {
    return err;
  }

  k_sleep(K_MSEC(30));
  return 0;
}

#define MPU6050_INIT(i)                                                        \
  static struct mpu6050_data mpu6050_data_##i;                                 \
                                                                               \
  static const struct mpu6050_config mpu6050_config_##i = {                    \
      .i2c = I2C_DT_SPEC_INST_GET(i),                                          \
      .gyro_fs = DT_INST_PROP(i, gyro_fs),                                     \
      .accel_fs = DT_INST_PROP(i, accel_fs),                                   \
      .smplrt_div = DT_INST_PROP(i, smplrt_div),                               \
      .dlpf_cfg = DT_INST_PROP(i, dlpf_cfg),                                   \
  };                                                                           \
                                                                               \
  SENSOR_DEVICE_DT_INST_DEFINE(i, mpu6050_init, NULL, &mpu6050_data_##i,       \
                               &mpu6050_config_##i, POST_KERNEL,               \
                               CONFIG_SENSOR_INIT_PRIORITY, &mpu6050_api);

DT_INST_FOREACH_STATUS_OKAY(MPU6050_INIT)
