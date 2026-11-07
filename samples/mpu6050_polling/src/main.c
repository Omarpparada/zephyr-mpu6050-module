#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
  const struct device *const dev = DEVICE_DT_GET_ONE(mpu6050);
  struct sensor_value accel[3];
  struct sensor_value gyro[3];
  double accel_m_s2[3];
  double gyro_deg_s[3];
  double attitude_deg[3] = {0.0, 0.0, 0.0};
  int64_t previous_ticks = 0;
  bool have_previous_sample = false;

  if (!device_is_ready(dev))
  {
    printk("Device not ready\n");
    return -ENODEV;
  }

  while (1)
  {
    int err = sensor_sample_fetch(dev);
    int64_t sample_ticks = k_uptime_ticks();

    if (err == 0)
    {
      err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    }
    if (err == 0)
    {
      err = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    }
    if (err < 0)
    {
      printk("MPU6050 read error: %d\n", err);
      return err;
    }

    double dt_s = have_previous_sample
                      ? (double)(sample_ticks - previous_ticks) /
                            CONFIG_SYS_CLOCK_TICKS_PER_SEC
                      : 0.0;

    for (int i = 0; i < 3; i++)
    {
      accel_m_s2[i] = sensor_value_to_double(&accel[i]);
      gyro_deg_s[i] = sensor_value_to_double(&gyro[i]) * 180000000.0 / SENSOR_PI;
      attitude_deg[i] += gyro_deg_s[i] * dt_s;
    }
    previous_ticks = sample_ticks;
    have_previous_sample = true;

    printk("accel_m_s2=[%.4f,%.4f,%.4f] "
           "gyro_deg_s=[%.4f,%.4f,%.4f] "
           "attitude_deg=[%.4f,%.4f,%.4f]\n",
           accel_m_s2[0], accel_m_s2[1], accel_m_s2[2],
           gyro_deg_s[0], gyro_deg_s[1], gyro_deg_s[2],
           attitude_deg[0], attitude_deg[1], attitude_deg[2]);

    k_sleep(K_MSEC(1000));
  }
}
