/* drivers/hwmon/mt6516/amit/IQS128.c - IQS128/PS driver
 * 
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

//#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_captouch.h>
#include "iqs128.h"

#include <captouch.h>
/******************************************************************************
 * configuration
*******************************************************************************/
#define IQS128_DEV_NAME     "IQS128"

/******************************************************************************
 * extern functions
*******************************************************************************/
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);

/*----------------------------------------------------------------------------*/
#define CAPTOUCH_EINT_TOUCH	(1)
#define CAPTOUCH_EINT_NO_TOUCH	(0)
/*----------------------------------------------------------------------------*/
static struct work_struct captouch_eint_work;
int captouch_eint_status=0;

/*-----------------------------------------------------------------------------*/
void iqs128_eint_func(void)
{
	//CAPTOUCH_LOG(" debug eint function performed!\n");
	schedule_work(&captouch_eint_work);
}

/*----------------------------------------------------------------------------*/
int iqs128_setup_eint(void)
{	
	mt_set_gpio_dir(GPIO_CAPTOUCH_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_CAPTOUCH_EINT_PIN, GPIO_CAPTOUCH_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_CAPTOUCH_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_CAPTOUCH_EINT_PIN, GPIO_PULL_UP);

	mt_eint_set_hw_debounce(CUST_EINT_CAPTOUCH_NUM, CUST_EINT_CAPTOUCH_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_CAPTOUCH_NUM, CUST_EINT_CAPTOUCH_TYPE, iqs128_eint_func, 0);

	mt_eint_unmask(CUST_EINT_CAPTOUCH_NUM);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static void iqs128_eint_work(struct work_struct *work)
{
	int err;

	if (captouch_eint_status == CAPTOUCH_EINT_NO_TOUCH)
	{
		captouch_eint_status = CAPTOUCH_EINT_TOUCH;
		if (CUST_EINT_CAPTOUCH_TYPE == CUST_EINTF_TRIGGER_LOW)
		{
			mt_eint_set_polarity(CUST_EINT_CAPTOUCH_NUM, 1);
		}
		else
		{
			mt_eint_set_polarity(CUST_EINT_CAPTOUCH_NUM, 0);
		}
	}
	else
	{
		captouch_eint_status = CAPTOUCH_EINT_NO_TOUCH;
		if (CUST_EINT_CAPTOUCH_TYPE == CUST_EINTF_TRIGGER_LOW)
		{
			mt_eint_set_polarity(CUST_EINT_CAPTOUCH_NUM, 0);
		}
		else
		{
			mt_eint_set_polarity(CUST_EINT_CAPTOUCH_NUM, 1);
		}
	}

	//let up layer to know
	if((err = captouch_report_interrupt_data(captouch_eint_status)))
	{
		CAPTOUCH_ERR("iqs128 call captouch_report_interrupt_data fail = %d\n", err);
	}	

	mt_eint_unmask(CUST_EINT_CAPTOUCH_NUM);
	
	return;
}
/*----------------------------------------------------------------------------*/
static int captouch_local_init(void) 
{
	struct captouch_control_path cap_ctl={0};
	int err = 0;

	CAPTOUCH_FUN();

	INIT_WORK(&captouch_eint_work, iqs128_eint_work);

	iqs128_setup_eint();

	err = captouch_register_control_path(&cap_ctl);
	if(err)
	{
		CAPTOUCH_ERR("captouch register fail = %d\n", err);
		return err;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int captouch_remove(void)
{
	CAPTOUCH_FUN();
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct captouch_init_info iqs128_init_info = {
		.name = IQS128_DEV_NAME,
		.init = captouch_local_init,
		.uninit = captouch_remove,
	
};
/*----------------------------------------------------------------------------*/
static int __init iqs128_init(void)
{
	CAPTOUCH_FUN();
	
	captouch_driver_add(&iqs128_init_info);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit iqs128_exit(void)
{
	CAPTOUCH_FUN();	
}
/*----------------------------------------------------------------------------*/
module_init(iqs128_init);
module_exit(iqs128_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Dexiang Liu");
MODULE_DESCRIPTION("IQS128 driver");
MODULE_LICENSE("GPL");
