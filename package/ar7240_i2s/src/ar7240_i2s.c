#include "ar7240_i2s.h"

///////////////////////////////////////////////////////////////////////////////////
//wm8918 i2c driver
///////////////////////////////////////////////////////////////////////////////////

#include "dm2016.h"
#include "wm8904.h"

ar7240_i2s_softc_t sc_buf_var; 


#define wm8918_i2c_addr 0x34

#define i2c_data 17
#define i2c_clk 1

#define SCL		(1 << 0)
#define SDA		(1 << 1)


#define delaytime  5

#define RTL_IIC_OUTPUT	1
#define RTL_IIC_INPUT	  0

static int wm__gpio_get_value(int gpio)
{
	if(ar7240_reg_rd(AR7240_GPIO_IN)&(1<<gpio))
	{
			return 1;
	}
	else
	{
			return 0;
	}
}

static void wm_gpio_direction_output(int gpio, int out)
{
		if(out)
		{
				ar7240_reg_rmw_set(AR7240_GPIO_OE, (1<<gpio));
		}
		else
		{
				ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<gpio));
		}
}

static void wm_gpio_direction_input(int gpio)
{
		ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<gpio));
}

static void wm__gpio_set_value(int gpio, int value)
{
		if(value)
			ar7240_reg_rmw_set(AR7240_GPIO_OUT, (1<<gpio));
		else
			ar7240_reg_rmw_clear(AR7240_GPIO_OUT, (1<<gpio));
}




  
/*  
 * I2C by GPIO simulated  set 1 routine. 
 * 
 * @param whichline: GPIO control line 
 * 
 */  

static void i2c_set_pin_dir(unsigned char Dir)
{
	if(Dir == RTL_IIC_OUTPUT)
	{
	//	wm_gpio_direction_output(i2c_clk,0);
		wm_gpio_direction_output(i2c_data,1);

	}
	else
	{
		wm_gpio_direction_input(i2c_data);

	}
}




static void i2c_clr(unsigned char whichline)  
{  
    unsigned char regvalue;  
      
    if(whichline == SCL)  
    {
				wm__gpio_set_value(i2c_clk, 0);
        return;  
    }  
    else if(whichline == SDA)  
    { 
				wm__gpio_set_value(i2c_data, 0);
        return;  
    }  
    else if(whichline == (SDA|SCL))  
    {  
				wm__gpio_set_value(i2c_clk, 0);
				wm__gpio_set_value(i2c_data, 0);
        return;  
    }  
    else  
    {  
        printk("Error input.\n");  
        return;  
    }  
      
}  




static void  i2c_set(unsigned char whichline)  
{  
    unsigned char regvalue;  
      
    if(whichline == SCL)  
    {
			  wm__gpio_set_value(i2c_clk, 1);
        return;  
    }  
    else if(whichline == SDA)  
    {  
				wm__gpio_set_value(i2c_data, 1);
        return;  
    }  
    else if(whichline == (SDA|SCL))  
    {  
				wm__gpio_set_value(i2c_data, 1);
				wm__gpio_set_value(i2c_clk, 1);
        return;  
    }  
    else  
    {  
        printk("Error input.\n");  
        return;  
    }  
}  

  

/*  
 * I2C by GPIO simulated  read data routine. 
 * 
 * @return value: a bit for read  
 * 
 */  
   
static unsigned char i2c_data_read(void)  
{  
	return wm__gpio_get_value(i2c_data);
}  

  
  
  
/* 
 * sends a start bit via I2C rountine. 
 * 
 */  
static void i2c_start_bit(void)  
{  
		i2c_set_pin_dir(RTL_IIC_OUTPUT);
    udelay(delaytime);  
    i2c_set(SDA | SCL);  
    udelay(delaytime);   
    i2c_clr(SDA);  
    udelay(delaytime*4);  
		i2c_clr(SCL);
		udelay(delaytime); 
}  


  
/* 
 * sends a stop bit via I2C rountine. 
 * 
 */  
static void i2c_stop_bit(void)  
{  
 		i2c_set_pin_dir(RTL_IIC_OUTPUT);
 //       udelay(delaytime);    
 //       i2c_clr(SCL);    
  
        /* actual stop bit */  
        udelay(delaytime);   
        i2c_clr(SDA);  
        udelay(delaytime);   
        i2c_set(SCL);  
        udelay(delaytime);   
        i2c_set(SDA);  
        udelay(delaytime);  
}  

  

  
/*  receives an acknowledge from I2C rountine. 
 * 
 *  @return value: 0--Ack received; 1--Nack received 
 *           
 */  
static int i2c_receive_ack(void)  
{  
    int nack;  
    unsigned char regvalue;  
	i2c_set_pin_dir(RTL_IIC_INPUT);        
      
 
      
//    udelay(delaytime);  
//    i2c_clr(SCL);  
    udelay(delaytime);   
    i2c_set(SCL);  
    udelay(delaytime);   
      
  
    nack = i2c_data_read();  
  
   udelay(delaytime);   
    i2c_clr(SCL);  
    udelay(delaytime);   
  
    if (nack == 0) 
    {
 	//	panic_printk("i2c ack error\n");
        return 1;   
    }
    return 0;  
}  


  
  
  
/*  
 * sends an acknowledge over I2C rountine. 
 * 
 */  
static void i2c_send_ack(unsigned char flag)  
{  
	i2c_set_pin_dir(RTL_IIC_OUTPUT);
 //   udelay(delaytime);  
//    i2c_clr(SCL);  
 if(flag)
	 i2c_set(SDA);
 else
	 i2c_clr(SDA);
    udelay(delaytime);  
    i2c_set(SCL);  
    udelay(delaytime);  
    i2c_clr(SCL);  
    udelay(delaytime);  
 
}  

  /* 
 * sends a character over I2C rountine. 
 * 
 * @param  c: character to send 
 * 
 */  
static int i2c_send_byte(unsigned char c)  
{  
    int i; 
	int ret;
	i2c_set_pin_dir(RTL_IIC_OUTPUT);
    local_irq_disable();  
    for (i=0; i<8; i++)  
    { 
        if (c & (1<<(7-i)))  
            i2c_set(SDA);  
        else  
            i2c_clr(SDA);  
  
        udelay(delaytime);   
        i2c_set(SCL); 
		udelay(delaytime);   
     	i2c_clr(SCL);
    }      
	ret=i2c_receive_ack();
    local_irq_enable();
	return ret;
}  


  
/*  receives a character from I2C rountine. 
 * 
 *  @return value: character received 
 * 
 */  
static unsigned char i2c_receive_byte(void)  
{  
    int j=0;  
    int i;  
    unsigned char regvalue;  
  	i2c_set_pin_dir(RTL_IIC_INPUT);
    local_irq_disable();  
    for (i=0; i<8; i++)  
    {  
        udelay(delaytime);   
        i2c_set(SCL);  
           
        udelay(delaytime);   
          
        if (i2c_data_read())  
            j+=(1<<(7-i));  
  
        udelay(delaytime);   
        i2c_clr(SCL);  
    }  
    local_irq_enable();  
    udelay(delaytime); 
  
    return j;  
}  



/*   
 *  read data from the I2C bus by GPIO simulated of a device rountine. 
 * 
 *  @param  devaddress:  address of the device 
 *  @param  address: address of register within device 
 *    
 *  @return value: data from the device readed 
 *  
 */  
  
int gpio_i2c_read(unsigned char devaddress, unsigned char address, unsigned char *rxdata,int count)  
{  
  //  int rxdata;  
	int i;
  
    i2c_start_bit();  
    if (!i2c_send_byte((unsigned char)(devaddress)))
    {
   		printk("gpio_i2c_read error 1\n");
		i2c_stop_bit();
		return -1;
	}  
    if (!i2c_send_byte(address))
    {
    	 printk("gpio_i2c_read error 2\n");
		i2c_stop_bit();
		return -1;

	}   
    i2c_start_bit();  
   if (! i2c_send_byte((unsigned char)(devaddress) | 1))
   	{
   		printk("gpio_i2c_read error 3\n");
	   i2c_stop_bit();
	   return -1;
   }
	for(i=0;i<count;i++)
	{
		*(rxdata+i)= i2c_receive_byte(); 
		if(i == (count-1))
			i2c_send_ack(1);
		else
			i2c_send_ack(0);  
	}
    i2c_stop_bit();  
    return 0;  
}  


  
/* 
 *  writes data to a device on the I2C bus rountine.  
 * 
 *  @param  devaddress:  address of the device 
 *  @param  address: address of register within device 
 *  @param  data:   data for write to device 
 * 
 */  
  
int gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char *data,int count)  
{
	int i;

	i2c_start_bit();  
    if (!i2c_send_byte((unsigned char)(devaddress)))
    {
    	printk("gpio_i2c_write error 1\n");
		i2c_stop_bit();
		return -1;
	}
    if (!i2c_send_byte(address))
     {
     	printk("gpio_i2c_write error 2\n");
		 i2c_stop_bit();
		 return -1;
	 }
	for(i=0;i<count;i++)
	{
	    if (! i2c_send_byte(*(data+i)))
	     {
	     	printk("gpio_i2c_write error xx %d\n",i);
			 i2c_stop_bit();
			 return -1;
		 }
	 }   
    i2c_stop_bit();
	return 0;
}  




/*   
*  read data from the I2C bus by GPIO simulated of a device rountine. 
* 
*  @param  devaddress:  address of the device 
*  @param  address: address of register within device 
*	
*  @return value: data from the device readed 
*  
*/ 

int wm8918_sigle_read(unsigned char reg, unsigned char *rxdata)  
{
#if 1
	int ret;
	ar7240_i2s_softc_t *sc = &sc_buf_var; 

	I2S_LOCK(sc);
	ret = gpio_i2c_read(wm8918_i2c_addr, reg, rxdata, 2);
	I2S_UNLOCK(sc); 
	return ret;
	
#else
	  int i;
  i2c_start_bit();	
  if (!i2c_send_byte((unsigned char)(devaddress)))
  {
	  printk("wm8904_sigle_read error 1\n");
		  i2c_stop_bit();
		  return -1;
	  }  
		
		if (!i2c_send_byte(reg))
		{
		   printk("wm8904_sigle_read error 2\n");
			   i2c_stop_bit();
			   return -1;
		  }   
	  i2c_start_bit();	
 if (! i2c_send_byte((unsigned char)(devaddress) | 1))
  {
	  printk("wm8904_sigle_read error 3\n");
	 i2c_stop_bit();
	 return -1;
 }
  
	  for(i=0;i<2;i++)
	  {
		  *(rxdata+i)= i2c_receive_byte();	
		  i2c_send_ack(0);	
	  }
  i2c_stop_bit();  
  return 0; 
#endif
}


int wm8918_multi_write(unsigned char reg, unsigned char *rxdata, int cnt)  
{
	return gpio_i2c_write(wm8918_i2c_addr, reg, rxdata, cnt*2);
}


int wm8918_multi_read(unsigned char reg, unsigned char *rxdata, int cnt)
{
	
}

int wm8918_sigle_write(unsigned char reg, unsigned short rxdata)  
{
#if 1

#ifdef WM8918_CODEC
	int ret;
	ar7240_i2s_softc_t *sc = &sc_buf_var; 
	unsigned char buf[2];
	
	I2S_LOCK(sc);
	buf[0]=(rxdata&0xff00)>>8;
	buf[1]=rxdata&0x00ff;
	ret = gpio_i2c_write(wm8918_i2c_addr, reg, buf, 2);
	I2S_UNLOCK(sc); 
	return ret;
#endif

#else
		int i;
    i2c_start_bit();  
    if (!i2c_send_byte((unsigned char)(devaddress)))
    {
   		printk("wm8904_sigle_write error 1\n");
			i2c_stop_bit();
			return -1;
		}  
		  
		  if (!i2c_send_byte(reg))
		  {
		  	 printk("wm8904_sigle_write error 2\n");
				 i2c_stop_bit();
				 return -1;
			}   
		  
		for(i=0;i<2;i++)
		{
		    if (! i2c_send_byte(*(rxdata+i)))
		     {
		     	printk("wm8904_sigle_write error xx %d\n",i);
				 i2c_stop_bit();
				 return -1;
			 }
		 }  
	 
    i2c_stop_bit();  
    return 0;  	
#endif
}




int rtl_i2c_test(void)
{
	int result;
	int try_time;
	char buf[5];
	unsigned short *id=(unsigned short *)buf;
	try_time=6;
	mdelay(500);
	while(try_time)
	{
		wm8918_sigle_read(WM8904_SW_RESET_AND_ID, buf);
		mdelay(100);
		printk("result =%x \n",*id);
		try_time--;
	}
	return 0;
}


/* 
 *  read data from the I2C bus by GPIO simulated of a digital camera device rountine. 
 * 
 *  @param  devaddress:  address of the device 
 *  @param  address: address of register within device 
 *   
 */
void init_i2c_port()
{

	//gpio_request(i2c_clk, "9331_i2c_control");
	//gpio_request(i2c_data, "9331_i2c_control");
	ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTIONS, (1<<7)); //SDA
	ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTION_2, (1<<10));
		
	wm_gpio_direction_output(i2c_clk, 1);
	wm_gpio_direction_output(i2c_data, 1);
	
	wm__gpio_set_value(i2c_clk, 1);
	wm__gpio_set_value(i2c_data, 1);
	while(0)
	{
		wm__gpio_set_value(i2c_clk, 1);
		wm__gpio_set_value(i2c_clk, 0);
			
		wm__gpio_set_value(i2c_data, 0);
	}
}


static int wm8904_reset(void)
{
	return wm8918_sigle_write(WM8904_SW_RESET_AND_ID, 0);
}



void InitWm8918(void)
{
	unsigned char buf[5];
	unsigned short *dbuf=(unsigned short *)buf;
	int ret;
	init_i2c_port();

	
	//rtl_i2c_test();
	wm8918_sigle_read(WM8904_SW_RESET_AND_ID, buf);
	//printf("");
	if(*dbuf != 0x8904)
	{
		printk("Device is not a WM8904, ID is %x\n", *dbuf);
		return;
	}

	//mdelay(10);
	ret = wm8918_sigle_read(WM8904_REVISION, buf);
	if(ret < 0)
	{
		printk("Failed to read device revision: %d\n",ret);
	}
	else
		printk("revision %c\n", ret + 'A');

	//mdelay(10);
	ret = wm8904_reset();
	if (ret < 0) {
		printk("Failed to issue reset\n");
		return;
	}
	#if 0
	wm8918_sigle_write(WM8904_CLOCK_RATES_2, 0x0006);
	wm8918_sigle_write(WM8904_BIAS_CONTROL_0, 0x000b);
	wm8918_sigle_write(WM8904_VMID_CONTROL_0, 0x0003);
	wm8918_sigle_write(WM8904_CLOCK_RATES_0, 0x845e);
	
	wm8918_sigle_write(WM8904_POWER_MANAGEMENT_2, 0x0003);
	wm8918_sigle_write(WM8904_POWER_MANAGEMENT_6, 0x000C);
	wm8918_sigle_write(WM8904_DAC_DIGITAL_1, 0);
	wm8918_sigle_write(WM8904_DC_SERVO_0, 0x0003);
	
	wm8918_sigle_write(WM8904_ANALOGUE_HP_0, 0x00ff);
	wm8918_sigle_write(WM8904_CHARGE_PUMP_0, 0x01);
	wm8918_sigle_write(WM8904_CLASS_W_0, 0x005);
	#else
#if 0
	//master mode
	wm8918_sigle_write(0x16, 0x0006);
	wm8918_sigle_write(0x04, 0x000b);
	wm8918_sigle_write(0x05, 0x0003);
	wm8918_sigle_write(0x14, 0x845e);
	wm8918_sigle_write(0x16, 0x4006);
	wm8918_sigle_write(0x0e, 0x0003);
	wm8918_sigle_write(0x12, 0x000c);
	wm8918_sigle_write(0x19, 0x404a);
	wm8918_sigle_write(0x1b, 0x0840);
	wm8918_sigle_write(0x3a, 0x000d);
	wm8918_sigle_write(0x43, 0x0003);
	wm8918_sigle_write(0x5a, 0x00ff);
	wm8918_sigle_write(0x62, 0x0001);
	wm8918_sigle_write(0x68, 0x0005);
	wm8918_sigle_write(0x62, 0x0001);
	wm8918_sigle_write(0x75, 0x0700);
	wm8918_sigle_write(0x76, 0x8fd5);
	wm8918_sigle_write(0x77, 0x00e0);
	wm8918_sigle_write(0x74, 0x0005);

#else
	//slave
	//mdelay(10);
	//i2s interface
	wm8918_sigle_write(WM8904_CLOCK_RATES_2, 0x0006);
	wm8918_sigle_write(WM8904_WRITE_SEQUENCER_0, 0x0100);
	wm8918_sigle_write(WM8904_WRITE_SEQUENCER_3, 0x0100);
	mdelay(500);
	wm8918_sigle_write(WM8904_CLOCK_RATES_0, 0x845e);
	wm8918_sigle_write(WM8904_ANALOGUE_OUT1_LEFT, 0x0039);
	wm8918_sigle_write(WM8904_ANALOGUE_OUT1_RIGHT, 0x00b9);
	wm8918_sigle_write(WM8904_DAC_DIGITAL_1, 0);
	wm8918_sigle_write(WM8904_CLASS_W_0, 0x0005);
	wm8918_sigle_write(0x19, 0x0002);
	
#endif

	ret = wm8918_sigle_read(WM8904_AUDIO_INTERFACE_0, buf);
	printk("WM8904_AUDIO_INTERFACE_0 is %x\n", *dbuf);
	ret = wm8918_sigle_read(WM8904_AUDIO_INTERFACE_1, buf);
	printk("WM8904_AUDIO_INTERFACE_1 is %x\n", *dbuf);
	ret = wm8918_sigle_read(WM8904_AUDIO_INTERFACE_2, buf);
	printk("WM8904_AUDIO_INTERFACE_2 is %x\n", *dbuf);
	ret = wm8918_sigle_read(WM8904_AUDIO_INTERFACE_3, buf);
	printk("WM8904_AUDIO_INTERFACE_3 is %x\n", *dbuf);	

	#endif
	
}



///////////////////////////////////////////////////////////////////////////////////
//atheros ar9331 i2s driver
///////////////////////////////////////////////////////////////////////////////////

int ar7240_i2s_major = 253; 
int ar7240_i2s_minor = 0; 
module_param(ar7240_i2s_major, int, S_IRUGO); 
module_param(ar7240_i2s_minor, int, S_IRUGO); 
 
 


ssize_t ar7240_i2s_read(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos);
ssize_t ar9100_i2s_write(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos);
ssize_t ar7240_i2s_write(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos, int resume);
int ar7240_i2s_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int ar7240_i2s_open(struct inode *inode, struct file *filp);
int ar7240_i2s_close(struct inode *inode, struct file *filp);
int ar7240_i2s_init(struct file *filp);
void ar7240_i2sound_dma_desc(unsigned long desc_buf_p, int mode);
void ar7240_i2sound_dma_start(int mode);
void ar7240_i2sound_dma_resume(int mode);
void ar7240_i2sound_request_dma_channel(void);
void ar7240_i2sound_dma_pause(int mode);
void ar7240_i2sound_dma_resume(int mode);
 
struct file_operations ar7240_i2s_fops = { 
	.owner = THIS_MODULE, 
	.read = ar7240_i2s_read, 
	.write = ar9100_i2s_write, 
	.unlocked_ioctl = ar7240_i2s_ioctl, 
	.open = ar7240_i2s_open, 
	.release = ar7240_i2s_close, 
}; 

irqreturn_t ar7240_i2s_intr(int irq, void *dev_id, struct pt_regs *regs) 
{ 
	uint32_t r; 
	static u_int16_t tglFlg = 0; 
	
	r = ar7240_reg_rd(MBOX_INT_STATUS);  /* Ack the interrupts */ 
	ar7240_reg_wr(MBOX_INT_STATUS, r); 
	return IRQ_HANDLED; 
} 

int ar7240_i2s_init_module(void) 
{ 
	 ar7240_i2s_softc_t *sc = &sc_buf_var; 
	 int result = -1; 
	 unsigned long rddata; 
	 

	 
	 memset(sc, 0,sizeof(ar7240_i2s_softc_t)); 
	 /* 
	  * Get a range of minor numbers to work with, asking for a dynamic 
	  * major unless directed otherwise at load time. 
	  */ 
	 if (ar7240_i2s_major) { 
	   result =  register_chrdev(ar7240_i2s_major, "ath_i2s", &ar7240_i2s_fops); 
	 } 
	 if (result < 0) { 
	  printk(KERN_WARNING "ar7240_i2s: can't get major %d\n", ar7240_i2s_major); 
	  return result; 
	 }  
	 sc->sc_irq = AR7240_MISC_IRQ_DMA; 
#if 0
	 /* Establish ISR would take care of enabling the interrupt */ 
	  result = request_irq(sc->sc_irq, ar7240_i2s_intr, AR7240_IRQF_DISABLED, "ar7240_i2s", NULL); 
	 if (result) { 
	  printk(KERN_INFO "i2s: can't get assigned irq %d returns %d\n", sc->sc_irq, result); 
	 } 
#endif
	/* AR9331 has 1 I2S interface but the output pin is configurable */ 
#ifdef I2S_GPIO_6_7_8_11_12 
	ar7240_reg_rmw_set(AR7240_GPIO_FUNCTIONS, 
	(AR7240_GPIO_FUNCTION_I2S_MCKEN | AR7240_GPIO_FUNCTION_I2S0_EN | 
	AR7240_GPIO_FUNCTION_JTAG_DISABLE)); 
	ar7240_reg_rmw_set(AR7240_GPIO_FUNCTION_2, (1<<9)); 
#else 
	
	//spidf GPIO23
// AR7240_GPIO_FUNCTION_I2S_MCKEN | 
	ar7240_reg_rmw_set(AR7240_GPIO_FUNCTIONS, 
	(AR7240_GPIO_FUNCTION_I2S_GPIO_18_22_EN | AR7240_GPIO_FUNCTION_I2S_MCKEN |
	AR7240_GPIO_FUNCTION_I2S0_EN | AR7240_GPIO_FUNCTION_JTAG_DISABLE
	));
	//| AR7240_GPIO_FUNCTION_SPDIF_EN)); 

	ar7240_reg_rmw_set(AR7240_GPIO_FUNCTION_2, (1<<2)); 
#endif 

	
	// Set GPIO_OE 
	ar7240_reg_rmw_set(AR7240_GPIO_OE, (1<<I2S_GPIOPIN_SCK) | (1<<I2S_GPIOPIN_WS) | 
	(1<<I2S_GPIOPIN_SD) | (1<<I2S_GPIOPIN_OMCLK)); 
	#ifdef MCLK_EXTERN_CLK
	//ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<I2S_GPIOPIN_SCK));	
	//ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<I2S_GPIOPIN_WS));		
	ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<I2S_GPIOPIN_OMCLK));	
	#endif
	ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1<<I2S_GPIOPIN_MIC)); 
	rddata = SAMP_44_1; //  for 44100 x 2 bytes x 2 channels = 176400 
	ar7240_reg_wr(AR7240_STEREO_CLK_DIV, rddata); 
	ar7240_reg_wr(AR7240_STEREO_CONFIG, (AR7240_STEREO_CONFIG_RESET | 
	EXTERL_CLK |
	AR7240_STEREO_CONFIG_SPDIF_ENABLE | 
	AR7240_STEREO_CONFIG_ENABLE | 
	AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B) | 
	AR7240_STEREO_CONFIG_SAMPLE_CNT_CLEAR_TYPE |  
	AR7240_STEREO_CONFIG_PSEDGE(2))); 
	udelay(100); 
	ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, AR7240_STEREO_CONFIG_RESET); 
	 I2S_LOCK_INIT(sc); 
	 	

	printk("gpio %x\n", ar7240_reg_rd(AR7240_GPIO_OE));

	 printk("stereo config %x\n", ar7240_reg_rd(AR7240_STEREO_CONFIG));
#ifdef WM8918_CODEC
	 InitWm8918();
#endif
	 return 0;  /* succeed */ 
} 
 
 
void ar7240_i2s_cleanup_module(void) 
{ 
	 ar7240_i2s_softc_t *sc = &sc_buf_var; 
	 printk(KERN_CRIT "unregister\n"); 
	 //free_irq(sc->sc_irq, NULL); 
	 unregister_chrdev(ar7240_i2s_major, "ath_i2s"); 
} 
 
 


int ar7240_i2s_open(struct inode *inode, struct file *filp) 
{ 
	 ar7240_i2s_softc_t *sc = &sc_buf_var; 
	 int opened = 0; 
	 
	 if ((filp->f_mode & FMODE_READ) && (sc->ropened)) { 
			printk("%s, %d I2S mic busy\n", __func__, __LINE__); 
	  	return -EBUSY; 
	 } 
	 if ((filp->f_mode & FMODE_WRITE) && (sc->popened)) { 
			printk("%s, %d I2S speaker busy\n", __func__, __LINE__); 
	  	return -EBUSY; 
	 } 
	 opened = (sc->ropened | sc->popened);  /* Reset MBOX FIFO's */ 
	 if (!opened) { 
	   	ar7240_reg_wr(MBOX_FIFO_RESET, 0xff); 
	  	udelay(500); 
	 } 
	 /* Allocate and initialize descriptors */ 
	 if (ar7240_i2s_init(filp) == ENOMEM) 
	  	return -ENOMEM; 
	 if (!opened) { 
	    ar7240_i2sound_request_dma_channel(); 
	    //ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, START);
	    //ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, RESUME);
	} 
	 return (0); 
} 
 

int ar7240_i2s_close(struct inode *inode, struct file *filp) 
{ 
		int j, own, mode; 
		ar7240_i2s_softc_t *sc = &sc_buf_var; 
		i2s_dma_buf_t *dmabuf; 
		ar7240_mbox_dma_desc *desc; 
		int status = TRUE; 
		int own_count = 0; 
		int flag=1;
		
		if (!filp) { 
			mode  = 0; 
		} 
		else 
		{ 
			mode = filp->f_mode; 
		} 
	 
	if (mode & FMODE_READ) 
	{ 
		dmabuf = &sc->sc_rbuf; 
		own = sc->rpause; 
		flag = 1;
	} 
	else 
	{ 
		dmabuf = &sc->sc_pbuf; 
		own = sc->ppause; 
		flag = 0;
	} 
	
	//ar7240_i2sound_dma_pause(flag);
	//ar7240_reg_wr(MBOX_FIFO_RESET, 0xff); 
	//udelay(500000);
	 //printk("00\n");
	desc = dmabuf->db_desc;  
	if(own) 
	{ 
	  for (j = 0; j < NUM_DESC; j++) 
	  { 
	   desc[j].OWN = 0; 
	  } 
	  //ar7240_i2sound_dma_resume(mode); 
	} 
	else 
	{ 
		for (j = 0; j < NUM_DESC; j++) 
		{ 
			if (desc[j].OWN) 
			{ 
				own_count++; 
			} 
		} 
	/* 
	 * The schedule_timeout_interruptible is commented 
	 * as this function is called from other process 
	 * context, i.e. that of wlan device driver context 
	 * schedule_timeout_interruptible(HZ); 
	 */ 
	 
		if (own_count > 0) 
		{ 
			//udelay((own_count * AOW_PER_DESC_INTERVAL) + 	DESC_FREE_WAIT_BUFFER); 
			for (j = 0; j < NUM_DESC; j++) 
			{ 
				/* break if the descriptor is still not free*/ 
				if (desc[j].OWN) 
				{ 
					status = FALSE; 
					printk("I2S : Fatal error\n"); 
					break; 
				} 
			} 
		} 
	} 
	 udelay(700000);
	 //printk("111\n");
	 for (j = 0; j < NUM_DESC; j++) 
	 { 
			dma_unmap_single(NULL, dmabuf->db_buf[j].bf_paddr,  I2S_BUF_SIZE, DMA_BIDIRECTIONAL); 
	 } 
 
 	//udelay(500000);
	// printk("222\n");
 	if (mode & FMODE_READ) 
 		kfree(sc->sc_rmall_buf); 
 	else 
		kfree(sc->sc_pmall_buf); 
		
 	dma_free_coherent(NULL, NUM_DESC * sizeof(ar7240_mbox_dma_desc), dmabuf->db_desc, dmabuf->db_desc_p); 
	if (mode & FMODE_READ) 
	{ 
		sc->ropened = 0; 
		sc->rpause = 0; 
	} 
	else 
	{ 
		sc->popened = 0; 
		sc->ppause = 0; 
	} 
  	//udelay(500000);
	 //printk("333\n");
  //ar7240_i2sound_dma_resume(flag);
 	return (status); 
}  


int ar7240_i2s_release(struct inode *inode, struct file *filp) 
{ 
 	printk(KERN_CRIT "release\n"); 
 	return 0; 
} 
 
 
 
 
 
 
ssize_t ar7240_i2s_read(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos) 
{ 
#define prev_tail(t) ({ (t == 0) ? (NUM_DESC - 1) : (t - 1); }) 
#define next_tail(t) ({ (t == (NUM_DESC - 1)) ? 0 : (t + 1); }) 
	uint8_t *data; 
	ssize_t retval; 
	struct ar7240_i2s_softc *sc = &sc_buf_var; 
	i2s_dma_buf_t *dmabuf = &sc->sc_rbuf; 
	i2s_buf_t *scbuf; 
	ar7240_mbox_dma_desc *desc; 
	unsigned int byte_cnt, mode = 1, offset = 0, tail = dmabuf->tail; 
	unsigned long desc_p; 
	int need_start = 0; 
	
	byte_cnt = count; 
 
	if (sc->ropened < 2) 
	{ 
		ar7240_reg_rmw_set(MBOX_INT_ENABLE, MBOX0_TX_DMA_COMPLETE); 
		need_start = 1; 
	} 
	
	sc->ropened = 2; 
	scbuf = dmabuf->db_buf; 
	desc = dmabuf->db_desc;  
	desc_p = (unsigned long) dmabuf->db_desc_p; 
	data = scbuf[0].bf_vaddr; 
	desc_p += tail * sizeof(ar7240_mbox_dma_desc); 
 while (byte_cnt && !desc[tail].OWN) 
 { 
		if (byte_cnt >= I2S_BUF_SIZE) 
		{ 
			desc[tail].length = I2S_BUF_SIZE; 
			byte_cnt -= I2S_BUF_SIZE; 
		} 
		else 
		{ 
			desc[tail].length = byte_cnt; 
			byte_cnt = 0; 
		} 
		
		dma_cache_sync(NULL, scbuf[tail].bf_vaddr, desc[tail].length, DMA_FROM_DEVICE); 
  	desc[tail].rsvd2 = 0; 
  	retval = copy_to_user(buf + offset, scbuf[tail].bf_vaddr, I2S_BUF_SIZE); 
  	if (retval) 
   		return retval; 
  	desc[tail].BufPtr = (unsigned int) scbuf[tail].bf_paddr; 

  	desc[tail].OWN = 1; 
  	tail = next_tail(tail); 
  	offset += I2S_BUF_SIZE; 
 } 
 
 dmabuf->tail = tail; 
 	if(need_start) 
 	{ 
		
 		ar7240_i2sound_dma_desc((unsigned long) desc_p, mode); 
 		if (filp) 
 		{ 
			ar7240_i2sound_dma_start(mode); 
		} 
	} 
	else if (!sc->rpause) 
	{ 
  	ar7240_i2sound_dma_resume(mode); 
	} 
	return offset; 
} 
 
ssize_t ar9100_i2s_write(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos) 
{ 
	int tmpcount, ret = 0; 
	int cnt = 0; 
	char *data; 
	unsigned long startTime ; 
	//char buf[50];
eagain: 
	tmpcount = count; 
	data = buf; 
	ret = 0; 
 	startTime = jiffies; 
 	//printk(KERN_WARNING "i2s write %d\n", count);
	//sprintf(buf, "echo \"i2s write %d\n\" > /tmp/i2s", count);
	//system(buf);
	do { 
w_agin:
		ret = ar7240_i2s_write(filp, data, tmpcount, f_pos, 1); 
		cnt++; 
		if (ret == EAGAIN) 
		{ 
			//printk("%s:%d %d\n", __func__, __LINE__, ret); 
			goto w_agin; 
		} 
		tmpcount = tmpcount - ret; 
		data += ret; 
	} while(tmpcount > 0); 
	
	return count; 
} 
 
 
ssize_t ar7240_i2s_write(struct file * filp, const char __user * buf, 
    size_t count, loff_t * f_pos, int resume) 
{ 
#define prev_tail(t) ({ (t == 0) ? (NUM_DESC - 1) : (t - 1); }) 
#define next_tail(t) ({ (t == (NUM_DESC - 1)) ? 0 : (t + 1); }) 
 
		uint8_t *data; 
		ssize_t retval; 
		int byte_cnt, offset, need_start = 0; 
		int mode = 0; 
		struct ar7240_i2s_softc *sc = &sc_buf_var; 
		i2s_dma_buf_t *dmabuf = &sc->sc_pbuf; 
		i2s_buf_t *scbuf; 
		ar7240_mbox_dma_desc *desc; 
		int tail = dmabuf->tail; 
		unsigned long desc_p; 
		int data_len = 0; 
 
 		I2S_LOCK(sc); 
 		byte_cnt = count; 
 		if (sc->popened < 2) 
 		{ 
			//printk("overflow\n");
 			ar7240_reg_rmw_set(MBOX_INT_ENABLE, MBOX0_RX_DMA_COMPLETE | RX_UNDERFLOW);   
 			need_start = 1; 
 		} 
 		
		sc->popened = 2; 
		scbuf = dmabuf->db_buf; 
		desc = dmabuf->db_desc; 
		desc_p = (unsigned long) dmabuf->db_desc_p; 
		offset = 0; 
		data = scbuf[0].bf_vaddr; 
 
 		desc_p += tail * sizeof(ar7240_mbox_dma_desc); 
 		
		//printk("write data: %d, %d, %d, %d\n", buf[0], buf[1], buf[2], buf[3]);
 		while (byte_cnt && !desc[tail].OWN) 
 		{ 
			if (byte_cnt >= I2S_BUF_SIZE) 
			{ 
   				desc[tail].length = I2S_BUF_SIZE; 
   				byte_cnt -= I2S_BUF_SIZE; 
				data_len = I2S_BUF_SIZE; 
  			} 
  			else 
  			{ 
   				desc[tail].length = byte_cnt; 
				data_len = byte_cnt; 
   				byte_cnt = 0; 
  			} 
  		
			if(!filp) 
			{ 
				memcpy(scbuf[tail].bf_vaddr, buf + offset, data_len); 
			} 
			else 
			{ 
				retval = copy_from_user(scbuf[tail].bf_vaddr, buf + offset, data_len); 
				if (retval) 
					return retval; 
			}
			 
			dma_cache_sync(NULL, scbuf[tail].bf_vaddr, desc[tail].length, DMA_TO_DEVICE); 
  			desc[tail].BufPtr = (unsigned int) scbuf[tail].bf_paddr; 
  			desc[tail].OWN = 1; 
  			tail = next_tail(tail); 
  			offset += data_len; 
 		} 
 
 		dmabuf->tail = tail; 
 		if (need_start) 
 		{ 
			//printk(KERN_WARNING "dma addr %x\n", desc_p);
			ar7240_i2sound_dma_desc((unsigned long) desc_p, mode); 
			ar7240_i2sound_dma_start(mode); 
		} 
		else if (!sc->ppause) 
		{ 
  			ar7240_i2sound_dma_resume(mode); 
 		} 
 		
 		I2S_UNLOCK(sc); 
 		return count - byte_cnt; 
} 

 #define VOLUME_MAX 0xc0
 
int ar7240_i2s_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
//int ar7240_i2s_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) 
{ 
#define AR7240_STEREO_CONFIG_DEFAULT (AR7240_STEREO_CONFIG_SPDIF_ENABLE | \ 
EXTERL_CLK | \
AR7240_STEREO_CONFIG_ENABLE | \ 
AR7240_STEREO_CONFIG_SAMPLE_CNT_CLEAR_TYPE | \ 
AR7240_STEREO_CONFIG_PSEDGE(2)) 
 
		int data, mask = 0, cab = 0, cab1 = 0, j, st_cfg = 0; 
		struct ar7240_i2s_softc *sc = &sc_buf_var; 
		i2s_dma_buf_t *dmabuf; 
		
	if (filp->f_mode & FMODE_READ) 
	{ 
		dmabuf = &sc->sc_rbuf; 
	} 
	else 
	{ 
		dmabuf = &sc->sc_pbuf; 
	} 
 
	switch (cmd) 
	{ 
		case I2S_MUTE:
				data = arg; 
				
				if(1 == data)
					wm8918_sigle_write(WM8904_DAC_DIGITAL_1,WM8904_DAC_MUTE);
				else
					wm8918_sigle_write(WM8904_DAC_DIGITAL_1,0);
				break;
 		case I2S_VOLUME:
				data = arg;
				if (data < 0) 
					data = 0;
				
				#if 0
				#else
				data = (data*VOLUME_MAX)/100;
				wm8918_sigle_write(WM8904_DAC_DIGITAL_VOLUME_LEFT,  0x100|data);
				wm8918_sigle_write(WM8904_DAC_DIGITAL_VOLUME_RIGHT, 0x100|data);
					
				#endif
				return 0;
		case I2S_FREQ:  /* Frequency settings */ 
  			data = arg; 
				switch (data) 
				{ 
					case 44100: 
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_44_1); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_44); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_44); 
						break; 
					case 48000: 
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_48); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48); 
						break; 
					case 36000:
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_36); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48); 
						break;
					case 24000:
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_24); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48); 
						break;
					case 22050:
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_22_05); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48); 
						break;	
					case 16000:
						ar7240_reg_wr(AR7240_STEREO_CLK_DIV, SAMP_16); 
						cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48); 
						cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48); 
						break; 
					default: 
						//printk("invalid sample rate %d\n", data);
						return -ENOTSUPP; 
				} 
				for (j = 0; j < NUM_DESC; j++) 
				{ 
					dmabuf->db_desc[j].Ca[0] |= cab; 
					dmabuf->db_desc[j].Cb[0] |= cab; 
					dmabuf->db_desc[j].Ca[1] |= cab1; 
					dmabuf->db_desc[j].Cb[1] |= cab1; 
				} 
				return 0; 
		case I2S_DSIZE: 
				data = arg; 
				switch (data) 
				{ 
					case 8: 
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_8B)); 
						cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16); 
						wm8918_sigle_write(0x19, 0x0002);
						break; 
					case 16: 
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B)); 
						cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16); 
						wm8918_sigle_write(0x19, 0x0002);
						break; 
					case 24: 
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_24B) | 
						AR7240_STEREO_CONFIG_I2S_32B_WORD); 
						cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32); 
						wm8918_sigle_write(0x19, 0x000a);
						break; 
					case 32: 
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_32B) | 
						AR7240_STEREO_CONFIG_I2S_32B_WORD); 
						cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32); 
						wm8918_sigle_write(0x19, 0x000e);
						break; 
					default: 
						printk(KERN_CRIT "Data size %d not supported \n", data); 
						return -ENOTSUPP; 
				} 
				ar7240_reg_wr(AR7240_STEREO_CONFIG, (st_cfg | AR7240_STEREO_CONFIG_RESET)); 
				udelay(100); 
				ar7240_reg_wr(AR7240_STEREO_CONFIG, st_cfg); 
				for (j = 0; j < NUM_DESC; j++) 
				{ 
					dmabuf->db_desc[j].Ca[1] |= cab1; 
					dmabuf->db_desc[j].Cb[1] |= cab1; 
				} 
				return 0; 
	case I2S_MODE:  /* mono or stereo */ 
  		data = arg; 
			/* For MONO */ 
			if (data != 2) 
			{ 
				ar7240_reg_rmw_set(AR7240_STEREO_CONFIG, MONO); 
			} 
			else
			{ 
				ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, MONO); 
			} 
			return 0; 
		default:   return -ENOTSUPP; 
	} 
} 
 
 

 

 
int ar7240_i2s_init(struct file *filp) 
{ 
		ar7240_i2s_softc_t *sc = &sc_buf_var; 
		i2s_dma_buf_t *dmabuf; 
		i2s_buf_t *scbuf; 
		uint8_t *bufp = NULL; 
		int j, byte_cnt, tail = 0, mode = 1; 
		ar7240_mbox_dma_desc *desc; 
		unsigned long desc_p; 
		int k;
 
		if (!filp) 
		{ 
			mode = FMODE_WRITE; 
		} 
		else 
		{ 
			mode = filp->f_mode; 
		} 
 
	if (mode & FMODE_READ) 
	{ 
		dmabuf = &sc->sc_rbuf; 
		sc->ropened = 1; 
		sc->rpause = 0; 
	} 
	else 
	{ 
		dmabuf = &sc->sc_pbuf; 
		sc->popened = 1; 
		sc->ppause = 0; 
	} 
	
  dmabuf->db_desc = (ar7240_mbox_dma_desc *) dma_alloc_coherent(NULL, NUM_DESC * sizeof(ar7240_mbox_dma_desc), &dmabuf->db_desc_p, GFP_DMA); 
	if (dmabuf->db_desc == NULL)
	{ 
		printk(KERN_CRIT "DMA desc alloc failed for %d\n", mode); 
		return ENOMEM; 
	} 
	
 	for (j = 0; j < NUM_DESC; j++) 
 	{ 
  	dmabuf->db_desc[j].OWN = 0; 
  	#ifdef SPDIF
			for (k = 0; k < 6; k++) {
				dmabuf->db_desc[j].Va[k] = 0;
				dmabuf->db_desc[j].Ua[k] = 0;
				dmabuf->db_desc[j].Ca[k] = 0;
				dmabuf->db_desc[j].Vb[k] = 0;
				dmabuf->db_desc[j].Ub[k] = 0;
				dmabuf->db_desc[j].Cb[k] = 0;
			}
#if 0
            /* 16 Bit, 44.1 KHz */
			dmabuf->db_desc[j].Ca[0] = 0x00100000;
			dmabuf->db_desc[j].Ca[1] = 0x000000f2;
			dmabuf->db_desc[j].Cb[0] = 0x00200000;
			dmabuf->db_desc[j].Cb[1] = 0x000000f2;
            /* 16 Bit, 48 KHz */
			dmabuf->db_desc[j].Ca[0] = 0x02100000;
			dmabuf->db_desc[j].Ca[1] = 0x000000d2;
			dmabuf->db_desc[j].Cb[0] = 0x02200000;
			dmabuf->db_desc[j].Cb[1] = 0x000000d2;
#endif
            /* For Dynamic Conf */
            dmabuf->db_desc[j].Ca[0] |= SPDIF_CONFIG_CHANNEL(SPDIF_MODE_LEFT);
            dmabuf->db_desc[j].Cb[0] |= SPDIF_CONFIG_CHANNEL(SPDIF_MODE_RIGHT);
#ifdef SPDIFIOCTL
			dmabuf->db_desc[j].Ca[0] = 0x00100000;
			dmabuf->db_desc[j].Ca[1] = 0x02100000;
			dmabuf->db_desc[j].Cb[0] = 0x00200000;
			dmabuf->db_desc[j].Cb[1] = 0x02100000;
#endif
#endif	   
 	} 
	/* Allocate data buffers */ 
	scbuf = dmabuf->db_buf; 
	if (!(bufp = kmalloc(NUM_DESC * I2S_BUF_SIZE, GFP_KERNEL))) 
	{ 
		printk(KERN_CRIT "Buffer allocation failed for \n"); 
		goto fail3; 
	} 
	if (mode & FMODE_READ) 
		sc->sc_rmall_buf = bufp; 
	else 
		sc->sc_pmall_buf = bufp; 
		
	for (j = 0; j < NUM_DESC; j++) 
	{ 
		scbuf[j].bf_vaddr = &bufp[j * I2S_BUF_SIZE]; 
		scbuf[j].bf_paddr = dma_map_single(NULL, scbuf[j].bf_vaddr, I2S_BUF_SIZE, DMA_BIDIRECTIONAL); 
	} 
	
	dmabuf->tail = 0; 
 // Initialize desc 
	desc = dmabuf->db_desc; 
	desc_p = (unsigned long) dmabuf->db_desc_p; 
	byte_cnt = NUM_DESC * I2S_BUF_SIZE; 
	tail = dmabuf->tail; 
	
	while (byte_cnt && (tail < NUM_DESC)) 
	{ 
		desc[tail].rsvd1 = 0; 
		desc[tail].size = I2S_BUF_SIZE; 
		if (byte_cnt > I2S_BUF_SIZE) 
		{ 
			desc[tail].length = I2S_BUF_SIZE;    
			byte_cnt -= I2S_BUF_SIZE; 
			desc[tail].EOM = 0; 
		} 
		else 
		{ 
			desc[tail].length = byte_cnt; 
			byte_cnt = 0; 
			desc[tail].EOM = 0; 
		} 
		desc[tail].rsvd2 = 0; 
		desc[tail].rsvd3 = 0; 
		desc[tail].BufPtr = (unsigned int) scbuf[tail].bf_paddr; 
		desc[tail].NextPtr = (desc_p + ((tail + 1) * (sizeof(ar7240_mbox_dma_desc))));
		 
		if (mode & FMODE_READ) 
		{ 
			desc[tail].OWN = 1; 
		} 
		else 
		{ 
			desc[tail].OWN = 0; 
		} 
		tail++; 
	} 
	
 	tail--; 
 	desc[tail].NextPtr = desc_p; 
 	dmabuf->tail = 0; 
 	return 0; 
fail3: 
	if (mode & FMODE_READ) 
		dmabuf = &sc->sc_rbuf; 
	else 
		dmabuf = &sc->sc_pbuf; 
 	dma_free_coherent(NULL, NUM_DESC * sizeof(ar7240_mbox_dma_desc),  dmabuf->db_desc, dmabuf->db_desc_p); 
	if (mode & FMODE_READ) 
	{ 
		if (sc->sc_rmall_buf) 
			kfree(sc->sc_rmall_buf); 
	} 
	else 
	{ 
		if (sc->sc_pmall_buf) 
			kfree(sc->sc_pmall_buf); 
	} 
 
 return -ENOMEM; 
} 
 

 
void ar7240_i2sound_request_dma_channel(void) 
{ 
	ar7240_reg_wr(MBOX_DMA_POLICY, 0x6a); 
} 
 
void ar7240_i2sound_dma_desc(unsigned long desc_buf_p, int mode) 
{ 
	/* 
	* Program the device to generate interrupts 
	* RX_DMA_COMPLETE for mbox 0 
	*/ 
	if (mode) 
	{ 
		//printk(KERN_WARNING "dma addr %x\n", desc_buf_p);
		ar7240_reg_wr(MBOX0_DMA_TX_DESCRIPTOR_BASE, desc_buf_p); 
	} 
	else 
	{ 
		ar7240_reg_wr(MBOX0_DMA_RX_DESCRIPTOR_BASE, desc_buf_p); 
	} 
} 
 
void ar7240_i2sound_dma_start(int mode) 
{ 
	/* 
	* Start 
	*/ 
	if (mode) 
	{ 
		ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, START); 
	} 
	else 
	{ 
		ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, START); 
	} 
} 
EXPORT_SYMBOL(ar7240_i2sound_dma_start); 
 
void ar7240_i2sound_dma_pause(int mode) 
{ 
/* 
  * Pause 
  */ 
	if (mode) 
	{ 
		ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, PAUSE); 
	} 
	else 
	{ 
		ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, PAUSE); 
	} 
} 

EXPORT_SYMBOL(ar7240_i2sound_dma_pause); 
 
void ar7240_i2sound_dma_resume(int mode) 
{ 
/* 
  * Resume 
  */ 
	if (mode) 
	{ 
		ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, RESUME); 
	} 
	else 
	{ 
		ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, RESUME); 
	} 
}
 
EXPORT_SYMBOL(ar7240_i2sound_dma_resume); 

MODULE_LICENSE("GPL"); 
module_init(ar7240_i2s_init_module);
module_exit(ar7240_i2s_cleanup_module);

