#ifndef __AR_H
#define  __AR_H

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/version.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#include <linux/delay.h>
#include "i2sio.h"
#include "ar7240.h"
 
#       define ar7240_dma_cache_sync(b, c)              \ 
        do {                                            \ 
                dma_cache_sync(NULL, (void *)b,         \ 
                                c, DMA_FROM_DEVICE);    \ 
        } while (0) 
#       define ar7240_cache_inv(d, s)                   \ 
        do {                                            \ 
                dma_sync_single(NULL, (dma_addr_t)d, s, \ 
                                DMA_TO_DEVICE);         \ 
        } while (0) 
#       define AR7240_IRQF_DISABLED     IRQF_DISABLED 

//#define MCLK_EXTERN_CLK

#define WM8918_CODEC

#ifdef MCLK_EXTERN_CLK

#define SAMP_44_1 ((0x0 << 16))
#define SAMP_48		((0x0 << 16))	

#define EXTERL_CLK (AR7240_STEREO_CONFIG_I2S_MCLK_SEL|AR7240_STEREO_CONFIG_MASTER)

#else

#define EXTERL_CLK (AR7240_STEREO_CONFIG_CLOCK_SEL|AR7240_STEREO_CONFIG_MASTER)

#if 1
#define SAMP_44_1 ((0x7 << 16) + 0xcb74)
#define SAMP_48		((0x7 << 16) + 0x2958)
#define SAMP_36   ((0x9 << 16) + 0x8c72)
#define SAMP_24   ((0xe << 16) + 0x52ac)
#define SAMP_22_05   ((0xf << 16) + 0x96f4)
#define SAMP_16		((0x11 << 16) + 0x8de8)

#else
#define SAMP_44_1 ((0x11 << 16) + 0xb75d)
#define SAMP_48		((0x10 << 16) + 0x46d0)
#endif
#endif

#define  SPDIF 
#undef  SPDIFIOCTL 
#define I2S 
#define MIC 
#define MBOX_DMA_POLICY          0xb80a0010 
#define MBOX0_DMA_RX_DESCRIPTOR_BASE 0xb80a0018 
#define MBOX0_DMA_RX_CONTROL  0xb80a001C 
#define MBOX0_DMA_TX_DESCRIPTOR_BASE 0xb80a0020 
#define MBOX0_DMA_TX_CONTROL  0xb80a0024 
#define MBOX_STATUS   0xb80a001c 
#define MBOX_INT_STATUS       0xb80a0044 
#define MBOX_INT_ENABLE          0xb80a004C 
#define MBOX_FIFO_RESET          0xb80a0058 
#define SPDIF_CONFIG_CHANNEL(x)                     ((3&x)<<20) 
#define SPDIF_MODE_LEFT                             1 
#define SPDIF_MODE_RIGHT                            2 
#define SPDIF_CONFIG_SAMP_FREQ(x)                   ((0xf&x)<<24) 
#define SPDIF_SAMP_FREQ_48                          2 
#define SPDIF_SAMP_FREQ_44                          0 
#define SPDIF_SAMP_FREQ_32													3
#define SPDIF_CONFIG_ORG_FREQ(x)                    ((0xf&x)<<4) 
#define SPDIF_ORG_FREQ_48                           0xd 
#define SPDIF_ORG_FREQ_44                           0xf 
#define SPDIF_CONFIG_SAMP_SIZE(x)                   (0xf&x) 
#define SPDIF_S_8_16                                2 
#define SPDIF_S_24_32                               0xb 
#define PAUSE    0x1 
#define START    0x2 
#define RESUME    0x4 
#define MBOX0_RX_DMA_COMPLETE  (1ul << 10) 
#define MBOX0_TX_DMA_COMPLETE  (1ul << 6) 
#define MONO                     (1ul << 14) 
#define MBOX0_RX_NOT_FULL      (1ul << 8) 
#define RX_UNDERFLOW                (1ul << 4) 
#define I2S_LOCK_INIT(_sc)  spin_lock_init(&(_sc)->i2s_lock) 
#define I2S_LOCK_DESTROY(_sc) 
#define I2S_LOCK(_sc)   spin_lock_irqsave(&(_sc)->i2s_lock, (_sc)->i2s_lockflags) 
#define I2S_UNLOCK(_sc)   spin_unlock_irqrestore(&(_sc)->i2s_lock, (_sc)->i2s_lockflags) 
//#define I2S_GPIO_6_7_8_11_12 
#ifdef I2S_GPIO_6_7_8_11_12 
#define I2S_GPIOPIN_MIC     12 
#define I2S_GPIOPIN_WS     7 
#define I2S_GPIOPIN_SCK     6 
#define I2S_GPIOPIN_SD     8 
#define I2S_GPIOPIN_OMCLK    11 
#else 
#define I2S_GPIOPIN_MIC     22 
#define I2S_GPIOPIN_WS     19 
#define I2S_GPIOPIN_SCK     18 
#define I2S_GPIOPIN_SD     20 
#define I2S_GPIOPIN_OMCLK    21 
#endif 
#define AOW_PER_DESC_INTERVAL 125      /* In usec */ 
#define DESC_FREE_WAIT_BUFFER 200      /* In usec */ 
#define TRUE    1 
#define FALSE   !(TRUE) 
#define AR7240_STEREO_CLK_DIV (AR7240_STEREO_BASE+0x1c) 
 
typedef struct { 
	unsigned int OWN  :  1,    /* bit 00 */ 
	EOM  :  1,    /* bit 01 */ 
	rsvd1     :  6,    /* bit 07-02 */ 
	size     : 12,    /* bit 19-08 */ 
	length     : 12,    /* bit 31-20 */ 
	rsvd2     :  4,    /* bit 00 */ 
	BufPtr     : 28,    /* bit 00 */ 
	rsvd3     :  4,    /* bit 00 */ 
	NextPtr : 28;    /* bit 00 */ 
	#ifdef SPDIF 
	unsigned int Va[6]; 
	unsigned int Ua[6]; 
	unsigned int Ca[6]; 
	unsigned int Vb[6]; 
	unsigned int Ub[6]; 
	unsigned int Cb[6]; 
	#endif 
	unsigned int pad; 
} ar7240_mbox_dma_desc; 
 
typedef struct i2s_buf { 
	 uint8_t *bf_vaddr; 
	 unsigned long bf_paddr; 
} i2s_buf_t; 
 
typedef struct i2s_dma_buf { 
	 ar7240_mbox_dma_desc *lastbuf; 
	 ar7240_mbox_dma_desc *db_desc; 
	 dma_addr_t db_desc_p; 
	 i2s_buf_t db_buf[NUM_DESC]; 
	 int tail; 
} i2s_dma_buf_t; 
 
typedef struct ar7240_i2s_softc { 
	 int ropened; 
	 int popened; 
	 i2s_dma_buf_t sc_pbuf; 
	 i2s_dma_buf_t sc_rbuf; 
	 char *sc_pmall_buf; 
	 char *sc_rmall_buf; 
	 int sc_irq; 
	 int ft_value; 
	 int ppause; 
	 int rpause; 
	 spinlock_t i2s_lock;   /* lock */ 
	 unsigned long i2s_lockflags; 
} ar7240_i2s_softc_t; 
#endif
