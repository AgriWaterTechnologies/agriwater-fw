idf_component_register(SRCS "main.c"
                            "./interfaces/wifi/src/wifi.c"
                            "./interfaces/mqtt/src/mqtt.c"
                            "./components/ultrasonic/ultrasonic.c"
                    INCLUDE_DIRS "./"
                                "./interfaces/wifi/inc/"
                                "./interfaces/mqtt/inc/"
                                "./components/ultrasonic/"
                                "./components/esp_idf_lib_helpers/")
