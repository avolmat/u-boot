// SPDX-License-Identifier: GPL-2.0+
/*
 * Author: Alain VOLMAT <avolmat@me.com>
 * Originaly based on board.c of stih410-b2260
 */

#include <common.h>
#include <cpu_func.h>
#include <init.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/spl.h>
#include <linux/usb/otg.h>
#include <dwc3-sti-glue.h>
#include <dwc3-uboot.h>
#include <usb.h>

#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include "btfmw_param.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SPL_BUILD
int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}
#endif

#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

int board_init(void)
{
	/*
	 * FIXME: MMC0 instance requires its flexgen clock to be
	 * enabled and connected to its correct parent.  Currently
	 * flexgen clock driver is not available hence DT properties
	 * related to clock cannot be handled.  To overcome that,
	 * perform the initialization manually and in an hardcoded
	 * manner here.
	 */
	/* Select MMC0 parent clock and enable it */
	writel(0x41414240, 0x9103020);
	writel(0x57, 0x910318c);

	return 0;
}

#ifdef CONFIG_USB_DWC3
static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
};

int dm_usb_gadget_handle_interrupts(struct udevice *dev)
{
	dwc3_uboot_handle_interrupt(dev);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int node;
	const void *blob = gd->fdt_blob;

	/* find the snps,dwc3 node */
	node = fdt_node_offset_by_compatible(blob, -1, "snps,dwc3");

	dwc3_device_data.base = fdtdec_get_addr(blob, node, "reg");

	return dwc3_uboot_init(&dwc3_device_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	dwc3_uboot_exit(index);
	return 0;
}

int g_dnl_board_usb_cable_connected(void)
{
	return 1;
}
#endif

#ifdef CONFIG_SPL_BUILD
/* Return the id of the core currently running */
static inline unsigned long read_mpidr(void)
{
	unsigned long val;

	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (val));

	return val;
}

/*
 * Currently SMP is not supported and moreover only
 * a single core must run in order to be able to
 * properly execute the btfmw for platform preparation
 *
 * s_init function is being called very early the
 * process of boot. Check for the coreid and "kill"
 * any other core than the core0
 *
 * TODO: this code is temporary until SMP is properly
 * handled
 */
void s_init(void)
{
	int val;

	/*
	 * Check core_id and only let Core0 proceed
	 * TODO - to be reworked, wfi or wfe and allow other
	 * cores to be waken-up, via psci ?
	 */
	val = read_mpidr();
	if (val & 0xff)
		while(1)
			wfi();
}

/*
 * board_init_f function is in charge of preparing the system
 * so that SDRAM and UART are accessible
 * btfmw_run takes care of that for us
 *
 * This function is based on board_init_f found in spl common
 * code, with the btfmw_run call done at the beginning
 *
 * Upon this call SDRAM is accessible and thus SDRAM can be
 * used
 */
extern int btfmw_run(void *param);
void board_init_f(ulong dummy)
{
	int ret;

	/* Execute the btfmw_run function to initialize the platform */
	ret = btfmw_run(btfmw_params_b2264stxh418rev1_bin);
	if (ret)
		panic("Failed to execute brfmw_run\n");

	ret = spl_early_init();
	if (ret)
		panic("spl_early_init() failed: %d\n", ret);

	preloader_console_init();
}

void spl_board_init(void)
{
	/*
	 * FIXME: MMC0 instance requires its flexgen clock to be
	 * enabled and connected to its correct parent.  Currently
	 * flexgen clock driver is not available hence DT properties
	 * related to clock cannot be handled.  To overcome that,
	 * perform the initialization manually and in an hardcoded
	 * manner here.
	 */
	/* Select MMC0 parent clock and enable it */
	writel(0x41414240, 0x9103020);
	writel(0x57, 0x910318c);
}

void board_boot_order(u32 *spl_boot_list)
{
	int i = 0;
#ifdef CONFIG_SPL_MMC
	spl_boot_list[i++] = BOOT_DEVICE_MMC1;
#endif
	spl_boot_list[i++] = BOOT_DEVICE_UART;
}

/* Empty function to reset the SPL */
void reset_cpu(void)
{
}
#endif