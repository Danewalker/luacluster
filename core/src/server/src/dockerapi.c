/* docker.c - worker thread
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

#include "plateform.h"
#include "dict.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "elog.h"
#include "sds.h"
#include "filesys.h"
#include "dicthelp.h"
#include "equeue.h"
#include "lvm.h"
#include "docker.h"
#include "proto.h"
#include "entityid.h"
#include "networking.h"
#include "int64.h"
#include "timesys.h"

//��Ϊ�������entity��client�Ķ�Ӧ��Ϣ��
//�������entity�ǵ�ǰ�ڵ��ֱ�ӷ��������
//����������ڵ��ʹ��upd���͵������ڵ������㴦��
//�ͻ��˷��͸����񣬷����͸��ͻ��ˣ������ڵ㷢�͸��ͻ��˶�ʹ�����Э��
static int luaB_SendToClient(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	//entity id
	unsigned long long did = luaL_tou64be(L, 1, 0, 0);
	unsigned long long pid = luaL_tou64be(L, 2, 0, 0);

	size_t ret;
	const char* pc = luaL_checklstring(L, 3, &ret);

	//����pid�Ĳ�ͬ���͵���ͬ�Ľڵ�
	//pid�ڵ�ǰ�ڵ㣬ʹ��tcp
	//pid�������ڵ㣬ʹ��udp
	DockerSendToClient(pVoid, did, pid, pc, ret);
	return 0;
}

static int luaB_CopyRpcToClient(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	DockerCopyRpcToClient(pVoid);
	return 0;
}

//���͸���������ĵ�����Ϣ
//1�� ͬһ���ڵ㲻ͬ�̡߳�2����ͬ�ڵ�
//ProtoRPC �ӽű��㴫���id�����������ġ�
//������ip�Ͷ˿ں����udpЭ�鷢��
static int luaB_Send(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	//entity id
	unsigned long long id = luaL_tou64be(L, 1, 0, 0);
	size_t ret;
	const char* pc = luaL_checklstring(L, 2, &ret);

	DockerSend(id, pc, ret);
	return 0;
}

static int luaB_Wait(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	unsigned long long msec = luaL_tou64be(L, 1, 0, 0);

	if (msec == 0)
		msec = INFINITE;

	return DockerLoop(pVoid, L, msec);
}

static int luaB_AllocateID(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	unsigned long long id = AllocateID(pVoid);
	luaL_u64pushnumber(L, id);
	return 1;
}

static int luaB_UnallocateID(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	unsigned long long id = luaL_tou64(L, 1);
	UnallocateID(pVoid, id);
	return 0;
}

//���ٺܼ򵥣�ֻҪ�ڽű���ɾ�����ɡ�
//entityproxy�����پ��鷳�ˣ�������߼��㲻���ơ�
//�ᵼ��ͨ��ʧ�ܣ�����ʧ�����첽ģʽ���������ġ�
//����entity��Ȼͬͨ��ĳ����ʽ������ϵ�������ַ�ʽ�ǿ����߶���ġ�
//��ôҲ��Ҫͨ��ĳ�ֶ���ķ�ʽȡ����ϵ��
//���첽ģʽ��ͨ��ʧ�ܳ�ʱ����ͨ���������ʽ������չ��
//���ʲô���ܶ�����ͨ���������ʽ������չ����������ȥ����
//socket��Ϊ�Ƕ�̬�����lib,���Ի��������̬�����lib��ͻ
//�����п�������һ��ͺ��ˡ�
static int luaB_CreateEntity(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	unsigned int type = luaL_checkinteger(L, 1);
	size_t ret;
	const char* pc = luaL_checklstring(L, 2, &ret);
	DockerCreateEntity(pVoid, type, pc, ret);
	return 0;
}

static int luaB_BindNet(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	unsigned short len = sizeof(ProtoNetBind);
	PProtoHead pProtoHead = malloc(len);
	pProtoHead->len = len;
	pProtoHead->proto = proto_net_bind;

	PProtoNetBind pProtoNetBind = (PProtoNetBind)pProtoHead;
	pProtoNetBind->entityId = luaL_tou64be(L, 1, 0, 0);
	pProtoNetBind->clientId = luaL_checkinteger(L, 2);

	SendToClient(0, (const char*)pProtoHead, len);
	free(pProtoHead);
	return 0;
}

static int luaB_DestoryNet(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	unsigned int clientId = luaL_checkinteger(L, 1);

	unsigned short len = sizeof(ProtoNetDestory);
	PProtoHead pProtoHead = malloc(len);
	pProtoHead->len = len;
	pProtoHead->proto = proto_net_destory;

	PProtoNetDestory pProtoNetDestory = (PProtoNetDestory)pProtoHead;
	pProtoNetDestory->clientId = luaL_checkinteger(L, 1);

	SendToClient(0, (const char*)pProtoHead, len);
	free(pProtoHead);
	return 0;
}

static int luaB_GetCurrentMilli(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	luaL_u64pushnumber(L, GetCurrentMilli());
	return 1;
}

static const luaL_Reg docker_funcs[] = {
	{"GetCurrentMilli", luaB_GetCurrentMilli},
	{"BindNet", luaB_BindNet},
	{"DestoryNet", luaB_DestoryNet},
	{"CreateEntity", luaB_CreateEntity},
	{"AllocateID", luaB_AllocateID},
	{"UnallocateID", luaB_UnallocateID},
	{"SendToClient", luaB_SendToClient},
	{"CopyRpcToClient", luaB_CopyRpcToClient},
	{"Send", luaB_Send},
	{"Wait", luaB_Wait},
	{NULL, NULL}
};

int LuaOpenDocker(lua_State* L) {
	luaL_register(L, "docker", docker_funcs);
	return 1;
}


