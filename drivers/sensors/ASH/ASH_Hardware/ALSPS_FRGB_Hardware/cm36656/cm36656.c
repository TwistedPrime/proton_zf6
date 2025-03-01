/* 
 * Copyright (C) 2014 ASUSTek Inc.
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

/*******************************************************/
/* ALSPS Front RGB Sensor CM36656 Module */
/******************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input/ASH.h>
#include <linux/regulator/consumer.h>
#include "cm36656.h"

/**************************/
/* Debug and Log System */
/************************/
#define MODULE_NAME			"ASH_HW"
#define SENSOR_TYPE_NAME		"ALSPS_FRGB"

#undef dbg
#ifdef ASH_HW_DEBUG
	#define dbg(fmt, args...) printk(KERN_DEBUG "[%s][%s]"fmt,MODULE_NAME,SENSOR_TYPE_NAME,##args)
#else
	#define dbg(fmt, args...)
#endif
#define log(fmt, args...) do {} while (0)
#define err(fmt, args...) printk(KERN_ERR "[%s][%s]"fmt,MODULE_NAME,SENSOR_TYPE_NAME,##args)

/*****************************************/
/*Global static variable and function*/
/****************************************/
static struct i2c_client	*g_i2c_client = NULL;

/*For Inititalization Function */
static int cm36656_proximity_hw_set_config(void);
static int cm36656_light_hw_set_config(void);

/*ALSPS FRGB Sensor Part*/
static int cm36656_ALSPS_FRGB_hw_check_ID(void);
static int cm36656_ALSPS_FRGB_hw_init(struct i2c_client* client);
static int cm36656_ALSPS_FRGB_hw_show_allreg(void);
static int cm36656_ALSPS_FRGB_hw_get_interrupt(void);
static int cm36656_ALSPS_FRGB_hw_set_register(uint8_t reg, int value);
static int cm36656_ALSPS_FRGB_hw_get_register(uint8_t reg);

/*Proximity Part*/
static int cm36656_proximity_hw_set_cs_config(uint8_t cs_conf_reg);
static int cm36656_proximity_hw_set_ps_start(uint8_t ps_start);
static int cm36656_proximity_hw_turn_onoff(bool bOn);
static int cm36656_proximity_hw_get_adc(void);
static int cm36656_proximity_hw_set_hi_threshold(int hi_threshold);
static int cm36656_proximity_hw_set_lo_threshold(int low_threshold);
static int cm36656_proximity_hw_set_force_mode(uint8_t force_mode);
static int cm36656_proximity_hw_set_led_current(uint8_t led_current_reg);
static int cm36656_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg);
static int cm36656_proximity_hw_set_persistence(uint8_t persistence);
static int cm36656_proximity_hw_set_integration(uint8_t integration);
static int cm36656_proximity_hw_set_autoK(int autok);
static int cm36656_regulator_init(void);
static int cm36656_regulator_enable(void);
static int cm36656_regulator_disable(void);

/* Light Sensor Part */
static int cm36656_light_hw_turn_onoff(bool bOn);
static int cm36656_light_hw_set_hi_threshold(int hi_threshold);
static int cm36656_light_hw_set_lo_threshold(int low_threshold);
static int cm36656_light_hw_set_persistence(uint8_t persistence);
static int cm36656_light_hw_set_integration(uint8_t integration);

/*FRGB sensor Part*/
static int cm36656_frgb_hw_get_red(void);
static int cm36656_frgb_hw_get_green(void);
static int cm36656_frgb_hw_get_blue(void);
static int cm36656_frgb_hw_get_ir(void);

/********************************/
/*For Inititalization Function */
/*******************************/
static int cm36656_ALSPS_FRGB_hw_check_ID(void)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/* Check the Device ID */
	ret = i2c_read_reg_u16(g_i2c_client, ID_REG, data_buf);
	if ((ret < 0) || (data_buf[0] != 0x57)) {
		err("%s ERROR(ID_REG : 0x%02X%02X). \n", __FUNCTION__, data_buf[1], data_buf[0]);
		return -ENOMEM;
	}
	log("%s Success(ID_REG : 0x%02X%02X). \n", __FUNCTION__, data_buf[1], data_buf[0]);
	return 0;
}

static int cm36656_proximity_hw_set_config(void)
{
	int ret = 0;
	ret = cm36656_proximity_hw_set_cs_config(CM36656_CS_START);
	if (ret < 0)		
		return ret;

	ret = cm36656_proximity_hw_set_led_current(CM36656_LED_I_110);
	if (ret < 0)		
		return ret;

	ret = cm36656_proximity_hw_set_led_duty_ratio(CM36656_PS_PERIOD_8);
	if (ret < 0)		
		return ret;

	ret = cm36656_proximity_hw_set_persistence(CM36656_PS_PERS_1);
	if (ret < 0)		
		return ret;

	ret = cm36656_proximity_hw_set_integration(CM36656_PS_IT_1T);
	if (ret < 0)		
		return ret;

	return 0;	
}

static int cm36656_light_hw_set_config(void)
{
	int ret = 0;	
	ret = cm36656_light_hw_set_persistence(CM36656_CS_PERS_1);
	if (ret < 0)		
		return ret;

	ret = cm36656_light_hw_set_integration(CM36656_CS_IT_50MS);
	if (ret < 0)		
		return ret;

	return 0;	
}

/*********************************/
/*ALSPS FRGB Sensor Part*/
/*******************************/
static int cm36656_ALSPS_FRGB_hw_init(struct i2c_client* client)
{
	int ret = 0;

	g_i2c_client = client;
	
	/* Check the Device ID 
	 * Do Not return when check ID
	 */
	ret = cm36656_ALSPS_FRGB_hw_check_ID();

	/*Init regulator setting */
	ret = cm36656_regulator_init();
	if (ret < 0) {
		return ret;
	}

	/*Set Proximity config */
	ret =cm36656_proximity_hw_set_config();
	if (ret < 0) {		
		return ret;
	}

	 /*Set Light Sensor config*/
	ret =cm36656_light_hw_set_config();
	if (ret < 0) {		
		return ret;
	}
	
	return 0;
}

static int cm36656_ALSPS_FRGB_hw_show_allreg(void)
{
	int ret = 0;	
	uint8_t buf[2] = {0};
	int reg = 0;
	for (; reg< 8;reg++)	
	{
		ret = i2c_read_reg_u16(g_i2c_client, reg, buf);
		log("Show All Register (0x%X) = 0x%02X%02X\n", reg, buf[1], buf[0]);
		if(ret < 0) 
		{
			err("%s: show all Register ERROR. (REG:0x%X)\n", __FUNCTION__, reg);
			return ret;
		}
	}
	for (reg=240;reg<247;reg++)	
	{
		ret = i2c_read_reg_u16(g_i2c_client, reg, buf);
		log("Show All Register (0x%X) = 0x%02X%02X\n", reg, buf[1], buf[0]);
		if(ret < 0) 
		{
			err("%s: show all Register ERROR. (REG:0x%X)\n", __FUNCTION__, reg);
			return ret;
		}
	}
	return 0;
}

static int cm36656_ALSPS_FRGB_hw_set_register(uint8_t reg, int value)
{	
	int ret = 0;	
	uint8_t buf[2] = {0};

	buf[1] = value/256;
	buf[0] = value%256;
	
	ret = i2c_write_reg_u16(g_i2c_client, reg, buf);
	log("Set Register Value (0x%X) = 0x%02X%02X\n", reg, buf[1], buf[0]);
	if(ret < 0) 
	{
		err("Set Register Value ERROR. (REG:0x%X)\n", reg);
		return ret;
	}
	
	return 0;
}

static int cm36656_ALSPS_FRGB_hw_get_register(uint8_t reg)
{
	int ret = 0;	
	uint8_t buf[2] = {0};
	int value;
	
	ret = i2c_read_reg_u16(g_i2c_client, reg, buf);
	log("Get Register Value (0x%X) = 0x%02X%02X\n", reg, buf[1], buf[0]);
	if(ret < 0) 
	{
		err("ALSPS FRGB Get Register Value ERROR. (REG:0x%X)\n", reg);
		return ret;
	}

	value =  buf[1]*256 + buf[0];
	
	return value;
}

static int cm36656_ALSPS_FRGB_hw_get_interrupt(void)
{
	uint8_t buf[2] = {0};
	bool check_flag = false;
	int alsps_frgb_int = 0;
	
	/* Read INT_FLAG will clean the interrupt */
	i2c_read_reg_u16(g_i2c_client, INT_FLAG, buf);

	/*Proximity Sensor work */
	if (buf[1]&INT_FLAG_PS_IF_AWAY || buf[1]&INT_FLAG_PS_IF_CLOSE) 
	{
		check_flag =true;
		if (buf[1]&INT_FLAG_PS_IF_AWAY) {
			alsps_frgb_int |= ALSPS_INT_PS_AWAY;	
		}else if (buf[1]&INT_FLAG_PS_IF_CLOSE) {		
			alsps_frgb_int |= ALSPS_INT_PS_CLOSE;
		}else {
			err("Can NOT recognize the Proximity INT_FLAG (0x%02X%02X)\n", buf[1], buf[0]);
			return -1;
		}
	}

	/* Light Sensor work */
	if(buf[1]&INT_FLAG_CS_IF_L || buf[1]&INT_FLAG_CS_IF_H) 
	{	
		check_flag =true;
		alsps_frgb_int |= ALSPS_INT_ALS;
	} 

	/* Interrupt Error */
	if(check_flag == false){		
//		err("Can NOT recognize the INT_FLAG (0x%02X%02X)\n", buf[1], buf[0]);
		return -1;
	}

	return alsps_frgb_int;
}

/******************/
/*Proximity Part*/
/******************/
static struct regulator *reg;

static int cm36656_regulator_init(void){
	int ret = 0;

	reg = regulator_get(&g_i2c_client->dev,"vcc_psensor");
    if (IS_ERR_OR_NULL(reg)) {
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
    ret = regulator_set_voltage(reg, 3000000, 3600000);
    if (ret) {
        err("Failed to set voltage for vcc_psensor reg %d\n", ret);
        return -1;
    }
    
    log("ldoc7 regulator setting init");
    return ret;
}

static int cm36656_regulator_enable(void){
    int ret = 0, idx = 0;

    if (IS_ERR_OR_NULL(reg)) {
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
    
    ret = regulator_set_load(reg, 10000);
    if (ret < 0) {
        err("Failed to set load for vcc_psensor reg %d\n", ret);
        return ret;
    }
    
    ret = regulator_enable(reg);
    if (ret) {
        err("Failed to enable vcc_psensor reg %d\n", ret);
        return -1;
    }
    
    for(idx=0; idx<10; idx++){
        if(regulator_is_enabled(reg) > 0){
            dbg("ldoc7 regulator is enabled(idx=%d)", idx);
            break;
        }
    }
    if(idx >= 10){
        err("ldoc7 regulator is enabled fail(tryty count >= %d)", idx);
        return -1;
    }
    
    log("Update ldoc7 to NPM_mode");
    return ret;
}

static int cm36656_regulator_disable(void){
	int ret = 0;

    if (IS_ERR_OR_NULL(reg)) {
        ret = PTR_ERR(reg);
        err("Failed to get regulator vcc_psensor %d\n", ret);
        return ret;
    }
    
    ret = regulator_set_load(reg, 0);
    if (ret < 0) {
        err("Failed to set load for vcc_psensor reg %d\n", ret);
        return ret;
    }
    
    ret = regulator_disable(reg);
    if (ret) {
        err("Failed to enable vincentr reg %d\n", ret);
        return -1;
    }
    
    log("Update ldoc7 to LPM_mode");
    return ret;
}

static int cm36656_proximity_hw_turn_onoff(bool bOn)
{
	static int PS_START = 0;
	int ret = 0;
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	if(bOn == 1){
		ret = cm36656_regulator_enable();
		if(ret < 0)
			return ret;
	} else {
		ret = cm36656_regulator_disable();
		if(ret < 0)
			return ret;
	}

	if(PS_START == 0){
		ret = cm36656_proximity_hw_set_ps_start(CM36656_PS_START);
		if (ret < 0)		
			return ret;
		ret = cm36656_proximity_hw_set_force_mode(0x8);
		if (ret < 0)
			return ret;
		PS_START = 1;
	}

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
	
	if (bOn == 1)	{	/* power on */		
		power_state_data_buf[0] &= CM36656_PS_SD_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity power on ERROR (PS_CONF1) \n");
			cm36656_regulator_disable();
			return ret;
		} else {
			log("Proximity power on (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	} else	{	/* power off */		
		power_state_data_buf[0] |= CM36656_PS_SD;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity power off ERROR (PS_CONF1) \n");
			cm36656_regulator_enable();
			return ret;
		} else {
			log("Proximity power off (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}
	
	return 0;
}

static int cm36656_proximity_hw_interrupt_onoff(bool bOn)
{
	int ret = 0;
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
	
	if (bOn == 1)	{	/* Enable INT */
		power_state_data_buf[0] |= CM36656_PS_INT_IN_AND_OUT;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity Enable INT ERROR (PS_CONF1) \n");
			return ret;
		} else {
			log("Proximity Enable INT (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	} else	{	/* Disable INT */		
      	power_state_data_buf[0] &= CM36656_PS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity Disable INT ERROR (PS_CONF1) \n");
			return ret;
		} else {
			log("Proximity Disable INT (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}
	
	return 0;
}


static int cm36656_proximity_hw_get_adc(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	

	ret = i2c_read_reg_u16(g_i2c_client, PS_DATA, adc_buf);
	if (ret < 0) {
		err("Proximity get adc ERROR. (PS_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Proximity get adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36656_proximity_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	
	/*Set Proximity High Threshold*/
	data_buf[0] = hi_threshold % (1<<8);
	data_buf[1] = hi_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDH, data_buf);
	if(ret < 0) {
		err("Proximity write High Threshold ERROR. (PS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	log("Proximity write High Threshold (PS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int cm36656_proximity_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/*Set Proximity Low Threshold*/	
	data_buf[0] = low_threshold % (1<<8);
	data_buf[1] = low_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDL, data_buf);
	if(ret < 0) {
		err("Proximity write Low Threshold ERROR. (PS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	log("Proximity write Low Threshold (PS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_cs_config(uint8_t cs_conf_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set cs config */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Proximity read SS_CONF ERROR\n");
		return ret;
	}
	dbg("Proximity read CS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	cs_conf_reg <<= CM36656_CS_START_SHIFT;
	data_buf[0] &= CM36656_CS_START_MASK;
	data_buf[0] |= cs_conf_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Proximity set CS start (CS_CONF) ERROR \n");
		return ret;
	} else {
		log("Proximity set CS start bit (CS_CONF : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_force_mode(uint8_t force_mode)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF3 ERROR\n");
		return ret;
	}
	log("Proximity read PS_CONF3 (0x%02X%02X)\n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	data_buf[0] |= force_mode;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if (ret < 0) {
		err("Proximity set Persistence (PS_CONF3) ERROR \n");
		return ret;
	} else {
		log("Proximity set Persistence (PS_CONF3 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_led_current(uint8_t led_current_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set LED config */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF4, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF4 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF4 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	led_current_reg <<= CM36656_LED_I_SHIFT;
	data_buf[1] &= CM36656_LED_I_MASK;
	data_buf[1] |= led_current_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF4, data_buf);
	if (ret < 0) {
		err("Proximity set LED Current (PS_CONF4) ERROR \n");
		return ret;
	} else {
		log("Proximity set LED Current (PS_CONF4 : 0x%X -> 0x%X) \n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity LED Duty Ratio */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	led_duty_ratio_reg <<= CM36656_PS_DR_SHIFT;
	data_buf[0] &= CM36656_PS_DR_MASK;
	data_buf[0] |= led_duty_ratio_reg;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity set LED Duty Ratio (PS_CONF1) ERROR \n");
		return ret;
	} else {
		log("Proximity set LED Duty Ratio (PS_CONF1 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static int cm36656_proximity_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	persistence <<= CM36656_PS_PERS_SHIFT;
	data_buf[0] &= CM36656_PS_PERS_MASK;
	data_buf[0] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity set Persistence (PS_CONF1) ERROR \n");
		return ret;
	} else {
		log("Proximity set Persistence (PS_CONF1 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	integration <<= CM36656_PS_IT_SHIFT;
	data_buf[1] &= CM36656_PS_IT_MASK;
	data_buf[1] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if (ret < 0) {
		err("Proximity set Integration (PS_CONF2) ERROR \n");
		return ret;
	} else {
		log("Proximity set Integration (PS_CONF2 : 0x%X -> 0x%X) \n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_ps_start(uint8_t ps_start)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF2 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF2 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	ps_start <<= CM36656_PS_START_SHIFT;
	data_buf[1] &= CM36656_PS_START_MASK;
	data_buf[1] |= ps_start;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF2, data_buf);
	if (ret < 0) {
		err("Proximity set PS start (PS_CONF2) ERROR \n");
		return ret;
	} else {
		log("Proximity set PS start (PS_CONF2 : 0x%X -> 0x%X) \n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int cm36656_proximity_hw_set_autoK(int autok)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	int hi_threshold, low_threshold;

	/*Get High threshold value and adjust*/
	ret = i2c_read_reg_u16(g_i2c_client, PS_THDH, data_buf);
	if (ret < 0) {
		err("Proximity get High Threshold ERROR. (PS_THDH)\n");
		return ret;
	}
	hi_threshold = (data_buf[1] << 8) + data_buf[0];
	dbg("Proximity get High Threshold : 0x%02X%02X\n", data_buf[1], data_buf[0]); 	
	cm36656_proximity_hw_set_hi_threshold(hi_threshold + autok);

	/*Get Low threshold value and adjust*/
	ret = i2c_read_reg_u16(g_i2c_client, PS_THDL, data_buf);
	if (ret < 0) {
		err("Proximity get Low Threshold ERROR. (PS_THDL)\n");
		return ret;
	}
	low_threshold = (data_buf[1] << 8) + data_buf[0];
	dbg("Proximity get Low Threshold : 0x%02X%02X\n", data_buf[1], data_buf[0]); 
	
	cm36656_proximity_hw_set_lo_threshold(low_threshold + autok);

	return 0;
}

/***********************/
/* Light Sensor Part */
/***********************/
static int cm36656_light_hw_turn_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);	
	if (ret < 0) {
		err("Light Sensor read CS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));

	if (bOn == 1)	{	/* power on */			
		power_state_data_buf[0] &= CM36656_CS_SD_MASK;

		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor power on ERROR (CS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor power on (CS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}		
	} else	{	/* power off */	
		power_state_data_buf[0] |= CM36656_CS_SD;
		
		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor power off ERROR (CS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor power off (CS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}	

	return 0;
}

static int cm36656_light_hw_interrupt_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);	
	if (ret < 0) {
		err("Light Sensor read CS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));

	if (bOn == 1)	{	/* Enable INT */
		power_state_data_buf[1] |= CM36656_CS_INT_EN;

		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor Enable INT ERROR (CS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor Enable INT (CS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[1], power_state_data_buf[1]);
		}		
	} else	{	/* Disable INT */	
      	power_state_data_buf[1] &= CM36656_CS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor Disable INT ERROR (CS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor Disable INT (CS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[1], power_state_data_buf[1]);
		}
	}	

	return 0;
}

static int cm36656_light_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor High Threshold*/	
	data_buf[0] = hi_threshold % (1<<8);
	data_buf[1] = hi_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, CS_THDH, data_buf);
	if(ret < 0) {
		err("[i2c] Light Sensor write High Threshold ERROR. (CS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	dbg("[i2c] Light Sensor write High Threshold (CS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int cm36656_light_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor Low Threshold*/		
	data_buf[0] = low_threshold % (1<<8);
	data_buf[1] = low_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, CS_THDL, data_buf);
	if(ret < 0) {
		err("[i2c] Light Sensor write Low Threshold ERROR. (CS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	dbg("[i2c] Light Sensor write Low Threshold (CS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int cm36656_light_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor read CS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	persistence <<= CM36656_CS_PERS_SHIFT;
	data_buf[1] &= CM36656_CS_PERS_MASK;
	data_buf[1] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor set Persistence (CS_CONF) ERROR \n");
		return ret;
	} else {
		log("Light Sensor set Persistence (CS_CONF : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36656_light_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor read CS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, sizeof(data_buf));

	integration <<= CM36656_CS_IT_SHIFT;
	data_buf[0] &= CM36656_CS_IT_MASK;
	data_buf[0] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor set Integration (CS_CONF) ERROR \n");
		return ret;
	} else {
		log("Light Sensor set Integration (CS_CONF : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static uint8_t cm36656_light_hw_get_integration(void)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, CS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor read CS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read CS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);

	data_buf[0] >>= CM36656_CS_IT_SHIFT;
	data_buf[0] &= CM36656_CS_IT_MAX;

	return data_buf[0];	
}

/***********************/
/*FRGB sensor Part*/
/**********************/
static int cm36656_frgb_hw_get_red(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	
	
	ret = i2c_read_reg_u16(g_i2c_client, CS_R_DATA, adc_buf);
	if (ret < 0) {
		err("Light Sensor Get adc ERROR (CS_R_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Light Sensor Get R adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36656_frgb_hw_get_green(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	
	
	ret = i2c_read_reg_u16(g_i2c_client, CS_G_DATA, adc_buf);
	if (ret < 0) {
		err("Light Sensor Get adc ERROR (CS_G_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Light Sensor Get G adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36656_frgb_hw_get_blue(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	
	
	ret = i2c_read_reg_u16(g_i2c_client, CS_B_DATA, adc_buf);
	if (ret < 0) {
		err("Light Sensor Get adc ERROR (CS_B_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Light Sensor Get B adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36656_frgb_hw_get_ir(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	
	
	ret = i2c_read_reg_u16(g_i2c_client, CS_IR_DATA, adc_buf);
	if (ret < 0) {
		err("Light Sensor Get adc ERROR (CS_IR_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Light Sensor Get IR adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}


static struct psensor_hw psensor_hw_cm36656 = {
	.proximity_low_threshold_default = CM36656_PROXIMITY_THDL_DEFAULT,
	.proximity_hi_threshold_default = CM36656_PROXIMITY_THDH_DEFAULT,
	.proximity_crosstalk_default = CM36656_PROXIMITY_INF_DEFAULT,
	.proximity_autok_min = CM36656_PROXIMITY_AUTOK_MIN,
	.proximity_autok_max = CM36656_PROXIMITY_AUTOK_MAX,
	
	.proximity_hw_turn_onoff = cm36656_proximity_hw_turn_onoff,
	.proximity_hw_interrupt_onoff = cm36656_proximity_hw_interrupt_onoff,
	.proximity_hw_get_adc = cm36656_proximity_hw_get_adc,
	.proximity_hw_set_hi_threshold = cm36656_proximity_hw_set_hi_threshold,
	.proximity_hw_set_lo_threshold = cm36656_proximity_hw_set_lo_threshold,
	.proximity_hw_set_autoK = cm36656_proximity_hw_set_autoK,
};

static struct lsensor_hw lsensor_hw_cm36656 = {
	.light_max_threshold = CM36656_LIGHT_MAX_THRESHOLD,
	.light_calibration_default = CM36656_LIGHT_CALIBRATION_DEFAULT,
		
	.light_hw_turn_onoff = cm36656_light_hw_turn_onoff,
	.light_hw_interrupt_onoff = cm36656_light_hw_interrupt_onoff,
	.light_hw_get_adc = cm36656_frgb_hw_get_green,
	.light_hw_set_hi_threshold = cm36656_light_hw_set_hi_threshold,
	.light_hw_set_lo_threshold = cm36656_light_hw_set_lo_threshold,
	.light_hw_set_integration = cm36656_light_hw_set_integration,
	.light_hw_get_integration = cm36656_light_hw_get_integration,
};

static struct FRGB_hw FRGB_hw_cm36656 = {		
	.frgb_hw_turn_onoff = cm36656_light_hw_turn_onoff,
	.frgb_hw_get_red = cm36656_frgb_hw_get_red,
	.frgb_hw_get_green = cm36656_frgb_hw_get_green,
	.frgb_hw_get_blue = cm36656_frgb_hw_get_blue,
	.frgb_hw_get_ir = cm36656_frgb_hw_get_ir,
};

static struct ALSPS_FRGB_hw ALSPS_FRGB_hw_cm36656 = {	
	.vendor = "Capella",
	.module_number = "cm36656",

	.ALSPS_FRGB_hw_check_ID = cm36656_ALSPS_FRGB_hw_check_ID,
	.ALSPS_FRGB_hw_init = cm36656_ALSPS_FRGB_hw_init,
	.ALSPS_FRGB_hw_get_interrupt = cm36656_ALSPS_FRGB_hw_get_interrupt,
	.ALSPS_FRGB_hw_show_allreg = cm36656_ALSPS_FRGB_hw_show_allreg,
	.ALSPS_FRGB_hw_set_register = cm36656_ALSPS_FRGB_hw_set_register,
	.ALSPS_FRGB_hw_get_register = cm36656_ALSPS_FRGB_hw_get_register,

	.mpsensor_hw = &psensor_hw_cm36656,
	.mlsensor_hw = &lsensor_hw_cm36656,
	.mFRGB_hw = &FRGB_hw_cm36656,
};

ALSPS_FRGB_hw* ALSPS_FRGB_hw_cm36656_getHardware(void)
{
	ALSPS_FRGB_hw* ALSPS_FRGB_hw_client = NULL;
	ALSPS_FRGB_hw_client = &ALSPS_FRGB_hw_cm36656;
	return ALSPS_FRGB_hw_client;
}
