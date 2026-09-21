SIMPLELINK_TRX_PLUGIN_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/imports.mak

CC = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
LNK = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
HEX = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-objcopy"

SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler gcc --product $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler gcc --product $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output . ../rfDualProtocolPacketTx.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = rfDualProtocolPacketTx.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_wisun.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk9p6kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk38p4kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_general_purpose_gfsk.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps_lp.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_sunoqpsk.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_8p33ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_16p67ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_66p67ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_xlr_20ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.obj source_ti_trx_host_hal_platform_spi.obj source_ti_trx_host_hal_platform_util.obj main_nortos.obj syscall_stubs.obj $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES)))

NAME = rfDualProtocolPacketTx


DFLAGS += -gstrict-dwarf

CFLAGS += -I.. \
    -I. \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    -Dxti_log_Log_ENABLE \
    "-I$(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source" \
    -DTRX_CC1407 \
    -DTRX_FEATURE_CC1407 \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/source" \
    -Os \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos" \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos/posix" \
    -mcpu=cortex-m4 \
    -march=armv7e-m \
    -mthumb \
    -std=c99 \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    -ffunction-sections \
    -fdata-sections \
    -g \
    -Wall \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include/newlib-nano" \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include"


ifneq ($(CONFIG),RELEASE)
# Add debug flags by default (excluded only when CONFIG=RELEASE)
CFLAGS += $(DFLAGS)
endif

LFLAGS += "-L$(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source" \
    -Wl,-l:ti/trx/host/lib/gcc/m4f/trx_host.a \
    -Wl,-T,../nortos/gcc/cc13x2x7_cc26x2x7_nortos.lds \
    "-Wl,-Map,$(NAME).map" \
    "-L$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/source" \
    -eresetISR \
    "-L$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos" \
    ti_utils_build_linker.cmd.genlibs \
    -l:ti/devices/cc13x2x7_cc26x2x7/driverlib/bin/gcc/driverlib.lib \
    -march=armv7e-m \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    -nostartfiles \
    -static \
    -Wl,--gc-sections \
    "-L$(GCC_ARMCOMPILER)/arm-none-eabi/lib/thumb/v7e-m/hard" \
    -lgcc \
    -lc \
    -lm \
    --specs=nano.specs \
    --specs=nosys.specs

all: $(NAME).hex

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo generation complete

syscfg: ../rfDualProtocolPacketTx.syscfg
	@ echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(@D) $<


# Helpful hint that the user needs to use a standalone SysConfig installation
$(SYSCONFIG_GUI_TOOL):
	$(error $(dir $(SYSCONFIG_TOOL)) does not contain the GUI framework \
        necessary to launch the SysConfig GUI.  Please set SYSCONFIG_TOOL \
        (in your SDK's imports.mak) to a standalone SysConfig installation \
        rather than one inside CCS)

syscfg-gui: ../rfDualProtocolPacketTx.syscfg $(SYSCONFIG_GUI_TOOL)
	@ echo Opening SysConfig GUI
	$(V) $(SYSCFG_GUI_CMD_STUB) $<


define C_RULE
$(basename $(notdir $(1))).obj: $(1) $(SYSCFG_H_FILES)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) $$< -c -o $$@
endef

$(foreach c_file,$(SYSCFG_C_FILES),$(eval $(call C_RULE,$(c_file))))

rfDualProtocolPacketTx.obj: ../rfDualProtocolPacketTx.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_wisun.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk50kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk9p6kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk9p6kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk38p4kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk38p4kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_general_purpose_gfsk.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_general_purpose_gfsk.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps_lp.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk50kbps_lp.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_sunoqpsk.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_sunoqpsk.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_8p33ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_8p33ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_16p67ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_16p67ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_66p67ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_66p67ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_xlr_20ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_xlr_20ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_host_hal_platform_spi.obj: $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source/ti/trx/host/hal/platform_spi.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

source_ti_trx_host_hal_platform_util.obj: $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source/ti/trx/host/hal/platform_util.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

main_nortos.obj: ../main_nortos.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

syscall_stubs.obj: ../syscall_stubs.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK)  $(OBJECTS)  $(LFLAGS) -o $(NAME).out

$(NAME).hex: $(NAME).out
	@ echo building $@
	$(V) $(HEX) -O ihex --remove-section .resetVecs --remove-section .dmaSpi0RxControlTableEntry --remove-section .dmaSpi0TxControlTableEntry --remove-section .dmaSpi1RxControlTableEntry --remove-section .dmaSpi1TxControlTableEntry --remove-section .dmaSpi0RxAltControlTableEntry --remove-section .dmaSpi0TxAltControlTableEntry --remove-section .dmaSpi1RxAltControlTableEntry --remove-section .dmaSpi1TxAltControlTableEntry $? $@

clean:
	@ echo Cleaning...
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).hex > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1
