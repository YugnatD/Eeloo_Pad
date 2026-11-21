idf_component_register(SRCS "main.c" "navball.c" "navball_simd.S" "hud.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_mm esp_lcd esp_driver_ppa esp_driver_uart esp_driver_usb_serial_jtag esp_timer)
# the renderer is the hot loop: build it with -O3
set_source_files_properties(navball.c PROPERTIES COMPILE_OPTIONS "-O3")
