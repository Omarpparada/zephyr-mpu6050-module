# zephyr-mpu6050-sensor

A custom MPU6050 driver for Zephyr OS, with a standalone polling example. The implementation uses the MPU6050 registers and the public Zephyr sensor API.

This repository contains the IMU driver and a simple test application.

## Driver

The driver checks the device identity, resets the sensor, and wakes it using the X-axis gyroscope as the clock reference. Ranges, sample-rate divider and low-pass filter come from devicetree. The defaults match the reset state, so an overlay that sets none of them behaves as before (+/-2 g, +/-250 deg/s, no filter).

| Property | Values | Default | Register |
| --- | --- | --- | --- |
| `gyro-fs` | 250, 500, 1000, 2000 deg/s | 250 | `GYRO_CONFIG.FS_SEL` |
| `accel-fs` | 2, 4, 8, 16 g | 2 | `ACCEL_CONFIG.AFS_SEL` |
| `smplrt-div` | 0-255 | 0 | `SMPLRT_DIV` |
| `dlpf-cfg` | 0-6 | 0 | `CONFIG.DLPF_CFG` |

A single 14-byte I2C burst reads acceleration, temperature, and gyroscope registers. Temperature is skipped. The six axis measurements are transformed into 16-bit integers and exposed through two channels:

| Zephyr channel | Output units | Conversion |
| --- | --- | --- |
| `SENSOR_CHAN_ACCEL_XYZ` | m/s2 | `raw * 9.80665 / (16384 >> AFS_SEL)` |
| `SENSOR_CHAN_GYRO_XYZ` | rad/s | `raw * pi / (180 * LSB per deg/s)` |

These conversion factors depend on the selected ranges. Acceleration includes gravity: at rest, its magnitude should be close to 9.81 m/s2, not zero. The driver returns errors to the caller; recovery is the application's responsibility.

## Polling Example

There is an example in `samples/mpu6050_polling`:

- Reads acceleration and angular velocity on all three axes.
- Converts the gyroscope output from rad/s to degrees/s.
- Integrates each axis using `angle[k] = angle[k-1] + angular_velocity[k] * dt`.
- Starts all angles at zero. The first sample establishes the time reference without updating the angles. `dt` uses the actual elapsed system ticks.
- Prints acceleration in m/s2, angular velocity in degrees/s, and angles in degrees.

## Hardware

The supplied overlay targets the Nucleo WL55JC and uses its `arduino_i2c` bus. Connect SDA/SCL to the corresponding bus pins, provide a common ground, and use power and I2C voltage levels compatible with the module and board. With AD0 low, the I2C address is `0x68`. With AD0 high, change the overlay node name and `reg` property to `0x69`. The expected `WHO_AM_I` value remains `0x68`.

```dts
&arduino_i2c {
	mpu6050@68 {
		compatible = "mpu6050";
		reg = <0x68>;
	};
};
```
