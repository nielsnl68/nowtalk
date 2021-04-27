set(srcs "wm8960.c")

idf_component_register(SRCS         "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES     i2c_bus
                                    nvs_flash)