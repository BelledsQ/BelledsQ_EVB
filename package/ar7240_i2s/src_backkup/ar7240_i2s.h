#include <asm/mach-ar7/ar7240.h>
#include <linux/delay.h>

#define  SPDIF
#undef  SPDIFIOCTL
#define I2S
#define MIC

#define VIRIAN

#define GPIO_BASE			0xb8040000
#define GPIO_FUNCTION	        	0xb8040028
#define GPIO_FUNCTION_2                 0xb8040030
#define AUDIO_PLL               	0xb8050030
#define FRAC_FREQ               	0xb8050034
#define RST_MISC_INTR_STATUS            0xb8060010
#define RST_RESET			0xb806001c

#define MBOX_DMA_POLICY	        	0xb80a0010
#define MBOX0_DMA_RX_DESCRIPTOR_BASE	0xb80a0018
#define MBOX0_DMA_RX_CONTROL		0xb80a001C
#define MBOX0_DMA_TX_DESCRIPTOR_BASE	0xb80a0020
#define MBOX0_DMA_TX_CONTROL		0xb80a0024
#define MBOX_STATUS			0xb80a001c
#define MBOX_INT_STATUS     		0xb80a0044
#define MBOX_INT_ENABLE	        	0xb80a004C
#define MBOX_FIFO_RESET	        	0xb80a0058

#define STEREO0_CONFIG		       	0xb80b0000
#define STEREO1_CONFIG      		0xb80b0004
#define STEREO0_VOLUME		       	0xb80b0008
#define STEREO1_VOLUME      		0xb80b000c
#if 1 /* Hornet */
#define STEREO_MASTER_CLOCK_reg         0xb80b001c
#else /* Wasp*/
#define STEREO_MASTER_CLOCK_reg         0xb80b0010
#endif

#define ATH_STEREO0_VOLUME       0xb80b0004
#define ATH_STEREO0_MASTER_CLOCK 0xb80b0008
#define ATH_STEREO0_TX_SAMPLE_CNT_LSB    0xb80b000c
#define ATH_STEREO0_TX_SAMPLE_CNT_MSB    0xb80b0010
#define ATH_STEREO0_RX_SAMPLE_CNT_LSB    0xb80b0014
#define ATH_STEREO0_RX_SAMPLE_CNT_MSB    0xb80b0018

#define SPDIF_CONFIG_CHANNEL(x)                     ((3&x)<<20)
#define SPDIF_MODE_LEFT                             1
#define SPDIF_MODE_RIGHT                            2
#define SPDIF_CONFIG_SAMP_FREQ(x)                   ((0xf&x)<<24)
#define SPDIF_SAMP_FREQ_48                          2
#define SPDIF_SAMP_FREQ_44                          0
#define SPDIF_CONFIG_ORG_FREQ(x)                    ((0xf&x)<<4)
#define SPDIF_ORG_FREQ_48                           0xd
#define SPDIF_ORG_FREQ_44                           0xf
#define SPDIF_CONFIG_SAMP_SIZE(x)                   (0xf&x)
#define SPDIF_S_8_16                                2
#define SPDIF_S_24_32                               0xb

#define PAUSE				0x1
#define START				0x2
#define RESUME				0x4

#define AUDIO_PLL_RESET     (1ul << 6)
#define STATUSMASK1			(1ul << 31)
#define STATUSMASK			(1ul << 4)
#define SHIFT_WIDTH			11
#define ADDR_VALID			1

#define MBOX0_RX_DMA_COMPLETE		(1ul << 10)
#define MBOX0_TX_DMA_COMPLETE		(1ul << 6)
#define MONO                    	(1ul << 14)
#define MBOX0_RX_NOT_FULL	    	(1ul << 8)
#define RX_UNDERFLOW                (1ul << 4)

#define I2S_LOCK_INIT(_sc)		spin_lock_init(&(_sc)->i2s_lock)
#define I2S_LOCK_DESTROY(_sc)
#define I2S_LOCK(_sc)			spin_lock_irqsave(&(_sc)->i2s_lock, (_sc)->i2s_lockflags)
#define I2S_UNLOCK(_sc)			spin_unlock_irqrestore(&(_sc)->i2s_lock, (_sc)->i2s_lockflags)

#define MASTER				HOWL
#define HOWL				1
#define DAC				0
#define AKM_DAC

#define GPIO_OUTPUT_MUX_NONE			 0
#define GPIO_OUTPUT_MUX_I2S_SCK 		12
#define GPIO_OUTPUT_MUX_I2S_WS 			13
#define GPIO_OUTPUT_MUX_I2S_SD 			14
#define GPIO_OUTPUT_MUX_I2S_MCK 		15
#define GPIO_OUTPUT_MUX_SPDIF 		25

#define GPIO_PIN0 0
#define GPIO_PIN3 3
#define GPIO_PIN11 11
#define GPIO_PIN14 14

//#define I2S_GPIO_6_7_8_11_12

#if 1 /* Hornet */
#ifdef I2S_GPIO_6_7_8_11_12
#define I2S_GPIOPIN_MIC					12
#define I2S_GPIOPIN_WS					7
#define I2S_GPIOPIN_SCK					6
#define I2S_GPIOPIN_SD					8
#define I2S_GPIOPIN_OMCLK				11
#else
#define I2S_GPIOPIN_MIC					22
#define I2S_GPIOPIN_WS					19
#define I2S_GPIOPIN_SCK					18
#define I2S_GPIOPIN_SD					20
#define I2S_GPIOPIN_OMCLK				21
#endif
#else /* Wasp */
#define I2S_GPIOPIN_MIC					GPIO_PIN11
#define I2S_GPIOPIN_WS					12
#define I2S_GPIOPIN_SCK					13
#define I2S_GPIOPIN_SD					14
#define I2S_GPIOPIN_OMCLK				15
#endif

#define EJTAG_DISABLE					2

typedef struct {
	unsigned int OWN		:  1,    /* bit 00 */
	             EOM		:  1,    /* bit 01 */
	             rsvd1	    :  6,    /* bit 07-02 */
	             size	    : 12,    /* bit 19-08 */
	             length	    : 12,    /* bit 31-20 */
	             rsvd2	    :  4,    /* bit 00 */
	             BufPtr	    : 28,    /* bit 00 */
	             rsvd3	    :  4,    /* bit 00 */
	             NextPtr	: 28;    /* bit 00 */
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

