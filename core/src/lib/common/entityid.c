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
* 1. �����ص����⣬��ԭ�������entity id��node�Ķ˿ڶ����̶�ʹ��9577��7795����ô�ڼ�����ϾͲ��ܿ������node���ڸ���entityid�Ĺ����޷��ҵ���Ӧ��node��
* ��Ϊ����ʹ�ö��̣߳����Բ����Ƿǳ����ص����⡣��һ̨��������Ҳ����ʹ�ö��̶߳����Ƕ࿪��������ĳЩ������������Ҫ�㿪��
* ��ʱ����Ҫһ��unsigned short����ʶ��ͬһ�������ϵĲ�ͬ�˿ڡ�����ó�һ��short����ʶ�˿ھ�Ҫ����entity id�ĳ��ȡ�
* ������»���uint64��ʹ����ôentity id��ֻʣ��255����Ȼ����ʹ�á�����һ����������Ȼʹ��uint64��
* һ��uint��ʾip,һ��char��ʾdock,һ��char��ʶ��upd�˿ڵ�ƫ�ơ�ʣ��һ��u short��ʶid������һ���߳������Դ���65535������
* 2. ��������֪��ָ���˿ںŷǳ���Ҫ����Ϊ����˿ڱ�ռ��Ӧ��Ҫ˳������������˿ڡ�����5597��ռ����ô�ͳ��Դ���5598ֱ������255��Ȼ����ʧ�ܾ�ֹͣ������
*/

#include "plateform.h"
#include "entityid.h"

#pragma pack(push,1)
typedef struct _EID {
	unsigned short id;
	unsigned char dock;
	unsigned char port;//UDP�˿ںŵ�ƫ��
	unsigned int addr;//ipv4
}*PEID, EID;
#pragma pack(pop)

unsigned char GetDockFromEID(VPEID pVPEID) {
	PEID pEID = (PEID) pVPEID;
	return pEID->dock;
}

void SetDockFromEID(VPEID pVPEID, unsigned char dock) {
	PEID pEID = (PEID)pVPEID;
	pEID->dock = dock;
}

unsigned short GetIDFromEID(VPEID pVPEID) {
	PEID pEID = (PEID)pVPEID;
	return pEID->id;
}

void SetIDFromEID(VPEID pVPEID, unsigned int id) {
	PEID pEID = (PEID)pVPEID;
	pEID->id = id;
}

void SetAddrFromEID(VPEID pVPEID, unsigned int addr) {
	PEID pEID = (PEID)pVPEID;
	pEID->addr = addr;
}

unsigned int GetAddrFromEID(VPEID pVPEID) {
	PEID pEID = (PEID)pVPEID;
	return pEID->addr;
}

void SetPortFromEID(VPEID pVPEID, unsigned char port) {
	PEID pEID = (PEID)pVPEID;
	pEID->port = port;
}

unsigned char GetPortFromEID(VPEID pVPEID) {
	PEID pEID = (PEID)pVPEID;
	return pEID->port;
}

VPEID CreateEID(unsigned int addr, unsigned char port, unsigned char dock, unsigned short id) {

	PEID pEID = (PEID)calloc(1, sizeof(EID));
	pEID->addr = addr;
	pEID->port = port;
	pEID->dock = dock;
	pEID->id = id;
	return pEID;
}

void DestoryEID(VPEID pVPEID) {
	PEID pEID = (PEID)pVPEID;
	free(pEID);
}

unsigned long long GetEID(VPEID pVPEID) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		unsigned long long ret = 0;
		memcpy(&ret, pVPEID, sizeof(EID));
		return ret;
	}
	return 0;
}

void SetEID(VPEID pVPEID, unsigned long long eid) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		memcpy(pVPEID, &eid, sizeof(EID));
	}
}

VPEID CreateEIDFromLongLong(unsigned long long eid) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		PEID pEID = (PEID)calloc(1, sizeof(EID));
		memcpy(pEID, &eid, sizeof(EID));
		return pEID;
	}
	return NULL;
}