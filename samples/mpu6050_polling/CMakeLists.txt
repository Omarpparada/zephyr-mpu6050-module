cmake_minimum_required(VERSION 3.20.0)

# Este repo no está en el manifiesto de west, así que Zephyr no lo descubre solo.
# Se añade como módulo extra: la ruta apunta a la raíz del repo (la carpeta que
# contiene zephyr/module.yml), dos niveles por encima de esta sample.
# Debe ir ANTES de find_package(Zephyr): zephyr_module.cmake lo lee ahí y, si
# llega tarde, el módulo no se carga sin dar error.
list(APPEND EXTRA_ZEPHYR_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Aquí Zephyr procesa los módulos (module.yml), el devicetree y Kconfig.
# A partir de esta línea ya existen los CONFIG_* como CONFIG_MPU6050.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mpu6050_polling)

# Fuentes de la aplicación. El driver NO se añade aquí: lo compila el
# CMakeLists del módulo si CONFIG_MPU6050=y.
target_sources(app PRIVATE src/main.c)
