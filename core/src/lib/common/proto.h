/* args.c - about args function
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


enum proto {
	//�ڵ�֮���Э��
	proto_rpc_create = 1,
	proto_rpc_call,//�����������������ת�����ͻ���
	proto_rpc_destory,//������������ӱ�����֪ͨdocker���ٶ�Ӧ��entity
	//ת�����ͻ��˵�Э��
	proto_route_call,//ֱ��ת�����ͻ��ˣ��������������߼�����
	//����Э��
	proto_ctr_cancel,//�����˳�
	proto_run_lua,//ִ��lua�ű�
	//���͸������
	proto_net_bind,//�����絽entityid
	proto_net_destory,//docker��Ӧ��entity�����٣�֪ͨ�����Ͽ�����
	proto_net_connect,//����һ������
	//����Э��
	proto_client_id,//ʹ�����緢��tcp��Ϣ��ָ��client id
	proto_client_entity,//ʹ�����緢��tcp��Ϣ��ָ��entity id
};

#pragma pack(push,1)
//Э��ͷ����������Ϊ��tcpЭ��
typedef struct _ProtoHead {
	unsigned short len;
	unsigned char proto;
}*PProtoHead, ProtoHead;

//��������Э��
typedef struct _ProtoRPCCreate {
	ProtoHead		protoHead;
	unsigned char callArg[];//��������Ĺ����ļ����ͳ�ʼ������
}*PProtoRPCCreate, ProtoRPCCreate;

//Զ�̵���Э��
typedef struct _ProtoRPC {
	ProtoHead		protoHead;
	unsigned long long id;
	unsigned char callArg[];//�����������Ƶ����ݰ�
}*PProtoRPC, ProtoRPC;

//ת���Ŀͻ��˵���Э��, �ͻ��˵��÷�����ֻ�ܵ��ö�Ӧsid����ת�������Բ���Ҫת��id
typedef struct _ProtoRoute {
	ProtoHead		protoHead;
	unsigned long long did;//Ŀ���id
	unsigned long long pid;//ת����proxy id
	unsigned char callArg[];
}*PProtoRoute, ProtoRoute;

//ת���Ŀͻ��˵���Э��, �ͻ��˵��÷�����ֻ�ܵ��ö�Ӧsid����ת�������Բ���Ҫת��id
typedef struct _ProtoRunLua {
	ProtoHead		protoHead;
	unsigned char luaString[];
}*PProtoRunLua, ProtoRunLua;

//docker���͸�����㣬��clientid
typedef struct _ProtoNetBind {
	ProtoHead		protoHead;
	unsigned int clientId;
	unsigned long long entityId;
}*PProtoNetBind, ProtoNetBind;

//docker���͸�����㣬����clientid
//���������proto_rpc_create��proto_rpc_destory
//������tcp��ص�entity
//�Ƿ���Ҫ����Ĺ������������Щ�����أ�
typedef struct _ProtoNetDestory {
	ProtoHead		protoHead;
	unsigned int clientId;
}*PProtoNetDestory, ProtoNetDestory;

//���Ͳ�ͬ���͵�tcpЭ�鵽�ͻ���
typedef struct _ProtoSendToClient {
	ProtoHead		protoHead;
	unsigned int	clientId;
	unsigned char buf[];
}*PProtoSendToClient, ProtoSendToClient;

typedef struct _ProtoSendToEntity {
	ProtoHead		protoHead;
	unsigned long long entityId;
	unsigned char buf[];
}*PProtoSendToEntity, ProtoSendToEntity;

typedef struct _ProtoSendTCancel {
	ProtoHead		protoHead;
}*PProtoSendTCancel, ProtoSendTCancel;

//Ŀǰ��֧��ipv6�ǵ�ȥ��
typedef struct _ProtoConnect {
	ProtoHead		protoHead;
	char ip[20];
	unsigned short port;
}*PProtoConnect, ProtoConnect;

#pragma pack(pop)