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

/*����*/
unsigned char EDesEn_Crypt( unsigned char *In_Out_Buffer,const unsigned char *IN_KEY );
/*����ֵ: 0      ��ʱpDataBuffer��ָ������ݻ������ڵ�ֵΪ��Ч��������                             */
/*        ��0    �����������ݻ�������ֵ��Ч��                                                      */

/*����˵��                                                                                           */
/*	pDataBuffer: ���ݻ�����������������8���ֽڣ�byte����                                             */
/*               ���ñ�����ʱ��������Ϊ�û���������ݣ�����Ϊ8���ֽڣ��������8���ֽڣ���������Ч��*/
/*               �������8���ֽ���ֻȡǰ8���ֽڣ�                                                    */
/*               ��������ʱ��������Ϊ�����Ľ����ǰ8���ֽ���Ч��                                   */

/*IN_KEY��       �û��趨����Կ����128λ��128 bits,16 bytes��,�粻��128λ����������Ч��            */
/*               �����128λ��ȡǰ128 λ���м���                                                     */


#ifdef __cplusplus
}
#endif


#endif /* ifndef __DM2016_H_ */
