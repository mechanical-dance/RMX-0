# lxr-0 module firmware: the desktop engine with the LXR-Drum module's I/O and UI.
# Output: LXRE_LXR0_MODULE.img (magic LXRERIMG). The engine, preset and MIDI object
# libraries are the desktop's, unchanged; the desktop UI, shift-register I/O and USB
# device init are replaced by the files in this directory.
# lxr_engine without TriggerOut.c: the desktop's trigger outputs and clock input drive pins
# the module uses for buttons and LEDs, and claim EXTI9_5 (module_triggers.c here).
add_library(lxr0_engine OBJECT
    ${PROJECT_SOURCE_DIR}/Core/Src/Soundengine/AudioCodecManager.c
    ${PROJECT_SOURCE_DIR}/Core/Src/Soundengine/cs4344_cs5343.c
    ${PROJECT_SOURCE_DIR}/Core/Src/Soundengine/FIFO.c
    ${PROJECT_SOURCE_DIR}/Core/Src/Soundengine/FxBus.c
    ${PROJECT_SOURCE_DIR}/Core/Src/Soundengine/Soundengine.c
)
target_link_libraries(lxr0_engine PUBLIC lxr_app_common)

add_executable(lxr0_module
    module_main.c
    module_hw.c
    module_it.c
    module_ui.c
    module_triggers.c
    module_panel.c
    module_display.c
    module_stubs.c
    module_kit.c
    ${PROJECT_SOURCE_DIR}/Core/Src/encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../bringup/cpu_meter.c
    ${PROJECT_SOURCE_DIR}/Core/Startup/startup_stm32f765vgtx.s
    ${PROJECT_SOURCE_DIR}/Core/Src/system_stm32f7xx.c
    ${PROJECT_SOURCE_DIR}/Core/Src/stm32f7xx_hal_msp.c
    ${PROJECT_SOURCE_DIR}/Core/Src/syscalls.c
    ${PROJECT_SOURCE_DIR}/Core/Src/sysmem.c
    ${PROJECT_SOURCE_DIR}/Core/Src/delay.c
    ${PROJECT_SOURCE_DIR}/Core/Src/lcd.c
    ${PROJECT_SOURCE_DIR}/Core/Src/Parameters.c
    ${PROJECT_SOURCE_DIR}/Core/Src/menu/Cc2Text.c
)
target_link_libraries(lxr0_module PRIVATE
    stm32f7_hal fatfs usb_midi
    lxr_preset lxr_dsp lxr0_engine lxr_midi lxr_sequencer lxr_usb_app
    lxr_app_common
)
target_compile_options(lxr0_module PRIVATE -Wall)
lxr_firmware_executable(lxr0_module LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/STM32F765VGTX_FLASH.ld)
lxr_add_firmware_outputs(lxr0_module IMAGE_MAGIC LXRERIMG IMAGE_NAME LXRE_LXR0_MODULE)
