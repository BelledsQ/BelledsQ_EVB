#ifndef __DM2016_H_
#define __DM2016_H_

#ifndef TRUE
#define TRUE            (1)
#endif

#ifndef FALSE
#define FALSE           (0)
#endif


#ifndef NULL
#define NULL            (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*加密*/
unsigned char EDesEn_Crypt( unsigned char *In_Out_Buffer,const unsigned char *IN_KEY );
/*返回值: 0      此时pDataBuffer所指向的数据缓冲区内的值为有效结算结果。                             */
/*        非0    计算有误，数据缓冲区内值无效。                                                      */

/*参数说明                                                                                           */
/*	pDataBuffer: 数据缓冲区，缓冲区最少8个字节（byte）。                                             */
/*               调用本函数时缓冲区内为用户输入的数据，最少为8个字节，如果不足8个字节，计算结果无效；*/
/*               如果多于8个字节则只取前8个字节；                                                    */
/*               函数返回时缓冲区内为计算后的结果，前8个字节有效。                                   */

/*IN_KEY：       用户设定的密钥，共128位（128 bits,16 bytes）,如不足128位，计算结果无效；            */
/*               如大于128位则取前128 位进行计算                                                     */


#ifdef __cplusplus
}
#endif


#endif /* ifndef __DM2016_H_ */
