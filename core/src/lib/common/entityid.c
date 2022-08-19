/* entity.c
*
* Copyright(C) 2021 - 2022, sun shuo <sun.shuo@surparallel.org>
* All rights reserved.
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or(at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.If not, see < https://www.gnu.org/licenses/>.
* 
* 1��NAN - Not A Number����˼�ǲ���һ����ֵ��VS������ʾ���ơ�1.#QNAN00000000000��������ԭ������������������縺���������жϷ���������float.hͷ�ļ�������_isnan()��ָ��λȫΪ1��β��λΪ�ա�
* 2��IND - Indeterminate Number����˼�ǲ�ȷ����ֵ��VS������ʾ���ơ�1.#IND000000000000������NAN��һ���������������ԭ��0��0�������޴�����޴��жϷ���ͨNAN��ָ��λȫΪ1��β��λ��Ϊ�ա�
* 3��INF - Infinity����˼�����޴�VS������ʾ���ơ�1.#INF000000000000��������ԭ����1/0.0�ļ��������жϷ�����_finite()�� 7ff��ͷ����λΪ0��
* 4��DEN - Denormalized����˼�Ƿǹ����ֵ��VS������ʾ���ơ�4.940656458421e-324#DEN����ָ��λȫΪ�㣬β��λ��Ϊ�ա�
* 5) �ǹ�񻯸�������denormalized number,a.k.a. subnormal number����ָ�ļ�����д����һ�����⸡�������ڹ�񻯸������У���������β����Ӧ������ǰ��0��
*/

#include "plateform.h"
#include "entityid.h"

//port max 127
unsigned char MakeUp(unsigned char h, unsigned char bit) {

	unsigned char bitHigh = bit;
	unsigned char bitLow = bit & 15;

	bitHigh = bitHigh >> 4;
	bitHigh = bitHigh << 5;
	bitHigh = bitHigh | bitLow;

	int ih = h & 127;
	int ibit = bitHigh & 224;

	if (((h & 127) == 127) && ((bitHigh & 224) == 224)) {
		bitHigh = bitHigh | 239;
	}
	else {
		bitHigh = bitHigh | 16;
	}
	return 	bitHigh;
}

unsigned char MakeDown(unsigned char bit) {
	unsigned char bitLow = bit & 15;

	bit = bit >> 5;
	bit = bit << 4;
	bit = bit | bitLow;

	return bit;
}
