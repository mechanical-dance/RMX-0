# lxr-0 bring-up image for the LXR-Drum module: LXRE_LXR0_BRINGUP.img (magic LXRERIMG).
add_executable(lxr0_bringup
    main.c
    it.c
    cpu_meter.c
    ${PROJECT_SOURCE_DIR}/Core/Src/lcd.c
    ${PROJECT_SOURCE_DIR}/Core/Src/delay.c
    ${PROJECT_SOURCE_DIR}/Core/Src/system_stm32f7xx.c
    ${PROJECT_SOURCE_DIR}/Core/Src/syscalls.c
    ${PROJECT_SOURCE_DIR}/Core/Src/sysmem.c
    ${PROJECT_SOURCE_DIR}/Core/Startup/startup_stm32f765vgtx.s
)
target_link_libraries(lxr0_bringup PRIVATE stm32f7_hal lxr_board_config)
lxr_firmware_executable(lxr0_bringup LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/STM32F765VGTX_FLASH.ld)
lxr_add_firmware_outputs(lxr0_bringup IMAGE_MAGIC LXRERIMG IMAGE_NAME LXRE_LXR0_BRINGUP)
