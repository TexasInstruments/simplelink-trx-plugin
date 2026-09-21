SIMPLELINK_TRX_PLUGIN_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/imports.mak

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
HEX = "$(TICLANG_ARMCOMPILER)/bin/tiarmobjcopy"

SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang --product $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler ticlang --product $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output . ../rfMdrTx.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = rfMdrTx.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_wisun.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk9p6kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk38p4kbps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_general_purpose_gfsk.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps_lp.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_sunoqpsk.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_8p33ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_16p67ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_66p67ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_xlr_20ksps.obj source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.obj source_ti_trx_wisun_wisun_delta_tables_NA.obj source_ti_trx_wisun_wisun_delta_tables_EU1.obj source_ti_trx_wisun_wisun_delta_tables_JP.obj source_ti_trx_wisun_wisun_delta_tables_BZ.obj source_ti_trx_host_hal_platform_spi.obj source_ti_trx_host_hal_platform_util.obj main_nortos.obj $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES)))

NAME = rfMdrTx


DFLAGS += -gdwarf-3

CFLAGS += -I.. \
    -I. \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    -Dxti_log_Log_ENABLE \
    "-I$(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source" \
    -DTRX_CC1407 \
    -DTRX_FEATURE_CC1407 \
    -Oz \
    -flto \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/source" \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos" \
    "-I$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos/posix" \
    -mcpu=cortex-m33 \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -Wall


ifneq ($(CONFIG),RELEASE)
# Add debug flags by default (excluded only when CONFIG=RELEASE)
CFLAGS += $(DFLAGS)
endif

LFLAGS += "-L$(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source" \
    -Wl,-lti/trx/host/lib/ticlang/m33f/trx_host.a \
    "-L$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/source" \
    "-L$(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR)/kernel/nortos" \
    ti_utils_build_linker.cmd.genlibs \
    -lti/devices/cc13x4_cc26x4/driverlib/bin/ticlang/driverlib.lib \
    ../ticlang/cc13x4_cc26x4_nortos.cmd \
    "-Wl,-m,$(NAME).map" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ARMCOMPILER)/lib" \
    -llibc.a

all: $(NAME).hex

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo generation complete

syscfg: ../rfMdrTx.syscfg
	@ echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(@D) $<


# Helpful hint that the user needs to use a standalone SysConfig installation
$(SYSCONFIG_GUI_TOOL):
	$(error $(dir $(SYSCONFIG_TOOL)) does not contain the GUI framework \
        necessary to launch the SysConfig GUI.  Please set SYSCONFIG_TOOL \
        (in your SDK's imports.mak) to a standalone SysConfig installation \
        rather than one inside CCS)

syscfg-gui: ../rfMdrTx.syscfg $(SYSCONFIG_GUI_TOOL)
	@ echo Opening SysConfig GUI
	$(V) $(SYSCFG_GUI_CMD_STUB) $<


define C_RULE
$(basename $(notdir $(1))).obj: $(1) $(SYSCFG_H_FILES)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach c_file,$(SYSCFG_C_FILES),$(eval $(call C_RULE,$(c_file))))

rfMdrTx.obj: ../rfMdrTx.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_wisun.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk50kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk9p6kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk9p6kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk38p4kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk38p4kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_general_purpose_gfsk.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_general_purpose_gfsk.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_2gfsk50kbps_lp.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk50kbps_lp.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_sunoqpsk.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_sunoqpsk.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_8p33ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_8p33ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_16p67ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_16p67ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_ofdmlr_66p67ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_ofdmlr_66p67ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_xlr_20ksps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_xlr_20ksps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_rfconfig_BP_EM_CC1407P_SE_rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.obj: ../source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_wisun_wisun_delta_tables_NA.obj: ../source/ti/trx/wisun/wisun_delta_tables_NA.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_wisun_wisun_delta_tables_EU1.obj: ../source/ti/trx/wisun/wisun_delta_tables_EU1.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_wisun_wisun_delta_tables_JP.obj: ../source/ti/trx/wisun/wisun_delta_tables_JP.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_wisun_wisun_delta_tables_BZ.obj: ../source/ti/trx/wisun/wisun_delta_tables_BZ.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_host_hal_platform_spi.obj: $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source/ti/trx/host/hal/platform_spi.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

source_ti_trx_host_hal_platform_util.obj: $(SIMPLELINK_TRX_PLUGIN_INSTALL_DIR)/source/ti/trx/host/hal/platform_util.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

main_nortos.obj: ../main_nortos.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK) -Wl,-u,_c_int00 $(OBJECTS)  $(LFLAGS) -o $(NAME).out

$(NAME).hex: $(NAME).out
	@ echo building $@
	$(V) $(HEX) -O ihex  $? $@

clean:
	@ echo Cleaning...
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
# Possible intermediate files for hex generation
	$(V) $(RM) $(NAME).a* > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).i* > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).m* > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).t* > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).x* > $(DEVNULL) 2>&1
# End of intermediate files
	$(V) $(RM) $(NAME).hex > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1
