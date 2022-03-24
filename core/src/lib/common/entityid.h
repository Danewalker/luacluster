/* entityid.h
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
*/

typedef void* VPEID;

//0 ��ʾ����1 ��ʾ������������ 11 λ�ǽ���, 52λ��β����
//����ȫ1��β��ȫ0��ʾ�����INF������1.0/0.0
//����ȫ1��β����ȫ0�ı�ʾ��Ч��NaN
#pragma pack(push,1)
typedef struct _EID {
	unsigned short id;
	unsigned int addr;//ipv4
	unsigned char dock;
	unsigned char port;//UDP�˿ںŵ�ƫ��
}*PEID, EID;

typedef union idl64
{
	volatile EID eid;
	volatile unsigned long long u;
	volatile double d;
} idl64;

typedef struct _UINTID {
	unsigned short work;
	unsigned short client;//UDP�˿ںŵ�ƫ��
}*PUINTID, UINTID;

typedef union idl32
{
	volatile UINTID id;
	volatile unsigned int u;
//	volatile float d;
} idl32;

#pragma pack(pop)

