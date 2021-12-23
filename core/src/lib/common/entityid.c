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

unsigned char GetDockFromEID(PEID pEID) {
	return pEID->dock;
}

void SetDockFromEID(PEID pEID, unsigned char dock) {
	pEID->dock = dock;
}

unsigned short GetIDFromEID(PEID pEID) {
	return pEID->id;
}

void SetIDFromEID(PEID pEID, unsigned int id) {
	pEID->id = id;
}

void SetAddrFromEID(PEID pEID, unsigned int addr) {
	pEID->addr = addr;
}

unsigned int GetAddrFromEID(PEID pEID) {
	return pEID->addr;
}

void SetPortFromEID(PEID pEID, unsigned char port) {
	pEID->port = port;
}

unsigned char GetPortFromEID(PEID pEID) {
	return pEID->port;
}

void CreateEID(PEID pEID, unsigned int addr, unsigned char port, unsigned char dock, unsigned short id) {
	pEID->addr = addr;
	pEID->port = port;
	pEID->dock = dock;
	pEID->id = id;
	return pEID;
}

unsigned long long GetEID(PEID pEID) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		unsigned long long ret = 0;
		memcpy(&ret, pEID, sizeof(EID));
		return ret;
	}
	return 0;
}

void SetEID(PEID pEID, unsigned long long eid) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		memcpy(pEID, &eid, sizeof(EID));
	}
}

void CreateEIDFromLongLong(unsigned long long eid, PEID pEID) {
	if (sizeof(EID) == sizeof(unsigned long long)) {
		memcpy(pEID, &eid, sizeof(EID));
	}
}