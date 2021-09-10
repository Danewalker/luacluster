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

//Sudoku

#include "plateform.h"
#include "dict.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "elog.h"
#include "sds.h"
#include "filesys.h"
#include "dicthelp.h"
#include "lvm.h"
#include "entityid.h"
#include "networking.h"
#include "int64.h"
#include "timesys.h"
#include "adlist.h"
#include "dict.h"
#include "transform.h"
#include "quaternion.h"
#include "vector.h"
#include "vector2.h"
#include "dicthelp.h"
#include <math.h>
#include "lua_cmsgpack.h"
#include "docker.h"
#include "int64.h"
#include "sudoku.h"
#include "rtree.h"
#include "rtreehelp.h"

#define OUTSIDESEC 60

enum entity_status {
	entity_normal = 0,
	entity_limite = 2,
	entity_outside = 4,//�Ѿ������߽�Ķ��󲻻�����κ��б����ᱻɾ����
	entity_ghost = 8
};

typedef struct _Entity {
	unsigned long long entityid;
	listNode* listNode;
	listNode* girdNode;//����grid_entity��nodeΪ�˿��ٲ��Һ�ɾ��node
	list* entityview;//dict-list ĳ��ʱ�������ӽǵ�����б�
	struct BasicTransform transform;
	struct Vector3 nowPotion;
	float velocity;//�ٶ�
	unsigned int stamp;
	unsigned int oldGird;//�ɵ�����id
	int update;//��һ֡���Ƿ��ƶ�������0û�У�1�ƶ���2�½���

	unsigned int status;//��ǰ����λ�ã�0��ͨ����2�ڱ߽�����4������Χ, 8���״̬�� ������ͼ��Χ�Ķ��󽫲����ٸ������ݡ�ֱ����ɾ����
	unsigned int outingStamp;
}*PEntity, Entity;

//�����൱����С������ϵͳ��������������������1��10����ô0��10����������1 * 1λ��, Ҳ���Ǳ��Ϊ1������
//����ͼԭ��Ҳ����begin�㲢��һ����0��0��Ҳ������-99��-99����ʱ����������Ͳ����ü򵥵ĳ˷�����ʾ�ˡ�
//�����������ꡣ��ȥbegin����������ֵ��Ȼ��������񳤶ȣ���ʹ��ceil����ȡ����Ȼ��x * y�������id
typedef struct _Sudoku {
	list* entitiesList;//������� Ҫ�������������entity Ϊ���еĲ�ѯ����
	dict* entitiesDict;//������� Ҫ�������������id��list entityָ��Ϊ�˵ڶ����ռ��ɼ���ң�
	dict* grid_entity;//dict-list �����ڵ�������ݡ�
	struct Vector3 gird;//����ĳ����Ǵ������ļ������õ�
	struct Vector3 begin, end;//��ͼԭ��ͽ����㣬��unityһ���Ǵ����½ǿ�ʼ�����Ͻ�
	unsigned int linex, maxid;

	//bigworld
	int isBigWorld;
	struct rtree* prtree;//��ͼ������������bigworld����
}*PSudoku, Sudoku;

static unsigned int GirdId(PSudoku pSudoku, struct Vector3* position) {
	unsigned int dx = (unsigned int)ceil((position->x - pSudoku->begin.x) / pSudoku->gird.x);
	unsigned int dz = (unsigned int)floor((position->z - pSudoku->begin.z) / pSudoku->gird.z);

	return dx + pSudoku->linex * dz;
}

enum SudokuDir {
	up = 0,
	down,
	left,
	right,
	upleft,
	upright,
	downleft,
	downright,
	dircentre,
	SudokuDirError
};

unsigned int GirdDirId(PSudoku pSudoku, enum SudokuDir dir, unsigned int centre) {

	unsigned int ret = 0;
	switch (dir)
	{
	case up:
		ret = centre + pSudoku->linex;
		if (ret > pSudoku->maxid) {
			ret = MAXUINT32;
		}
		break;
	case down:
if (centre <= pSudoku->linex)
ret = MAXUINT32;
else
ret = centre - pSudoku->linex;
break;
	case left:
		if (floor(centre / pSudoku->linex) == floor((centre - 1) / pSudoku->linex))
			ret = centre - 1;
		else
			ret = MAXUINT32;
		break;
	case right:
		if (floor(centre / pSudoku->linex) == floor((centre + 1 - 0.001) / pSudoku->linex))
			ret = centre + 1;
		else
			ret = MAXUINT32;
		break;
	case upleft:
		if (MAXUINT32 == GirdDirId(pSudoku, up, centre) || MAXUINT32 == GirdDirId(pSudoku, left, centre))
			ret = MAXUINT32;
		else
			ret = centre + pSudoku->linex - 1;
		break;
	case upright:
		if (MAXUINT32 == GirdDirId(pSudoku, up, centre) || MAXUINT32 == GirdDirId(pSudoku, right, centre))
			ret = MAXUINT32;
		else
			ret = centre + pSudoku->linex + 1;
		break;
	case downleft:
		if (MAXUINT32 == GirdDirId(pSudoku, down, centre) || MAXUINT32 == GirdDirId(pSudoku, left, centre))
			ret = MAXUINT32;
		else
			ret = centre - pSudoku->linex - 1;
		break;
	case downright:
		if (MAXUINT32 == GirdDirId(pSudoku, down, centre) || MAXUINT32 == GirdDirId(pSudoku, right, centre))
			ret = MAXUINT32;
		else
			ret = centre - pSudoku->linex + 1;
		break;
	case dircentre:
		ret = centre;
		break;

	default:
		break;
	}

	return ret;
}

int GirdDir(void* pSudoku, unsigned int entry, unsigned int centre) {

	for (int i = up; i <= downright; i++) {

		if (entry == GirdDirId(pSudoku, i, centre))
			return i;
	}

	return SudokuDirError;
}

int GirdLimite(void* pSudoku, struct Vector3* position) {

	unsigned int gid = GirdId(pSudoku, position);

	if (GirdDirId(pSudoku, up, gid) != MAXUINT32) {
		return false;
	}
	else if (GirdDirId(pSudoku, down, gid) != MAXUINT32) {
		return false;
	}
	else if (GirdDirId(pSudoku, left, gid) != MAXUINT32) {
		return false;
	}
	else if (GirdDirId(pSudoku, right, gid) != MAXUINT32) {
		return false;
	}

	return true;
}

void SudokuMove(void* pSudoku, unsigned long long id, struct Vector3 position, struct Vector3 rotation, float velocity, unsigned int stamp) {
	PSudoku s = (PSudoku)pSudoku;

	if (position.x < s->begin.x || position.x > s->end.x || position.z < s->begin.z || position.z > s->end.z) {
		n_error("sudoku::move entry out limit %U", id);
		return;
	}

	//���Ҷ�Ӧ��id
	dictEntry* entry = dictFind(s->entitiesDict, &id);

	if (entry == NULL) {
		n_error("sudoku::move error not find id %U", id);
	}

	PEntity pEntity = dictGetVal(entry);

	if (GirdId(s, &pEntity->transform.position) != GirdId(s, &position)) {
		SudokuEntry(s, id, position, rotation, velocity, stamp, (pEntity->status & entity_ghost));
		return;
	}

	//position
	pEntity->transform.position.x = position.x;
	pEntity->transform.position.z = position.z;
	pEntity->nowPotion.x = position.x;
	pEntity->nowPotion.z = position.z;

	//rotation
	rotation.y = rotation.y * RADIANS_PER_DEGREE;

	quatEulerAngles(&rotation, &pEntity->transform.rotation);

	//velocity ÿ���ƶ����ٶ�
	pEntity->velocity = velocity;

	//stamp ��
	pEntity->stamp = stamp;

	if (pEntity->status & entity_outside) {
		pEntity->status &= ~entity_outside;
		pEntity->outingStamp = 0;
	}
}

static int luaB_Move(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);
	unsigned long long id = luaL_tou64(L, 2);

	//position
	struct Vector3 position = { 0 };
	position.x = luaL_checknumber(L, 3);
	position.z = luaL_checknumber(L, 4);

	//rotation
	struct Vector3 rotation = { 0 };
	rotation.y = luaL_checknumber(L, 5);

	//velocity ÿ���ƶ����ٶ�
	float velocity = luaL_checknumber(L, 6);

	//stamp ��
	float stamp = luaL_checkinteger(L, 7);

	SudokuMove(s, id, position, rotation, velocity, stamp);
	return 0;
}

void SudokuLeave(void* pSudoku, unsigned long long id) {
	PSudoku s = (PSudoku)pSudoku;

	//���Ҷ�Ӧ��id
	dictEntry* entry = dictFind(s->entitiesDict, &id);

	if (entry == NULL) {
		n_error("sudoku::move error not find id %U", id);
	}
	PEntity pEntity = dictGetVal(entry);

	listDelNode(s->entitiesList, pEntity->listNode);

	unsigned int gird = GirdId(s, &pEntity->transform.position);

	entry = dictFind(s->grid_entity, &gird);
	list* girdlist = NULL;
	if (entry == NULL || gird == MAXUINT32) {
		n_error("sudoku::move error not find gird id %i", gird);
	}
	else {
		girdlist = dictGetVal(entry);
	}
	listDelNode(girdlist, pEntity->girdNode);

	if (listLength(girdlist) == 0) {
		dictDelete(s->grid_entity, &gird);
	}

	listRelease(pEntity->entityview);
	free(pEntity);
}

static int luaB_Leave(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);
	unsigned long long id = luaL_tou64(L, 2);

	SudokuLeave(s, id);
	return 0;
}

void SudokuEntry(void* pSudoku, unsigned long long id, struct Vector3 position, struct Vector3 rotation, float velocity, unsigned int stamp, unsigned int isGhost) {
	PSudoku s = (PSudoku)pSudoku;

	if (position.x < s->begin.x || position.x > s->end.x || position.z < s->begin.z || position.z > s->end.z) {

		n_error("entry out limit %U", id);
		return;
	}

	PEntity pEntity;
	dictEntry* entry = dictFind(s->entitiesDict, &id);
	if (entry != NULL) {
		pEntity = dictGetVal(entry);
	}
	else {
		pEntity = calloc(1, sizeof(Entity));
	}

	pEntity->entityid = id;
	pEntity->entityview = listCreate();

	//position
	pEntity->transform.position.x = position.x;
	pEntity->transform.position.z = position.z;
	pEntity->nowPotion.x = position.x;
	pEntity->nowPotion.z = position.z;

	//rotation
	rotation.y = rotation.y * RADIANS_PER_DEGREE;

	quatEulerAngles(&rotation, &pEntity->transform.rotation);

	//velocity ÿ���ƶ����ٶ�
	pEntity->velocity = velocity;

	//stamp ��
	pEntity->stamp = stamp;

	pEntity->update = 2;

	pEntity->oldGird = GirdId(s, &pEntity->nowPotion);

	listAddNodeHead(s->entitiesList, pEntity);
	pEntity->listNode = listFirst(s->entitiesList);
	dictAddWithLonglong(s->entitiesDict, id, pEntity);

	//����gird
	unsigned int gird = GirdId(s, &pEntity->transform.position);
	entry = dictFind(s->grid_entity, &gird);
	list* girdlist = NULL;
	if (entry == NULL) {
		girdlist = listCreate();
		dictAddWithUint(s->grid_entity, gird, girdlist);
	}
	else {
		girdlist = dictGetVal(entry);
	}

	listAddNodeHead(girdlist, pEntity);
	pEntity->girdNode = listFirst(girdlist);


	if (s->isBigWorld) {
		if (isGhost) {
			pEntity->status |= entity_ghost | entity_limite;
		}
		else if (GirdLimite(s, &position)) {
			pEntity->status |= entity_limite;
		}
	}
}

static int luaB_Entry(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);
	unsigned long long id = luaL_tou64(L, 2);

	//position
	struct Vector3 position = { 0 };
	position.x = luaL_checknumber(L, 3);
	position.z = luaL_checknumber(L, 4);

	//rotation
	struct Vector3 rotation = { 0 };
	rotation.y = luaL_checknumber(L, 5);

	//velocity ÿ���ƶ����ٶ�
	float velocity = luaL_checknumber(L, 6);

	//stamp ��
	float stamp = luaL_checkinteger(L, 7);

	int isGhost = luaL_checkinteger(L, 8);

	SudokuEntry(s, id, position, rotation, velocity, stamp, isGhost);
	return 0;
}

void FillView(PSudoku s, enum SudokuDir dir, unsigned int centre, PEntity pEntityEntry) {

	unsigned int viewgird = GirdDirId(s, dir, centre);
	if (MAXUINT32 == viewgird) {
		//n_error("FillView not GirdDirId gird %i", viewgird);
		return;
	}

	dictEntry* entry = dictFind(s->grid_entity, &viewgird);
	if (entry == NULL) {
		//n_error("FillView not find gird %i", viewgird);
	}
	else {
		list* newview = dictGetVal(entry);

		listIter* iter = listGetIterator(newview, AL_START_HEAD);
		listNode* node;
		while ((node = listNext(iter)) != NULL) {
			PEntity pEntity = listNodeValue(node);

			if (pEntity->entityid != pEntityEntry->entityid) {
				if(!(s->isBigWorld && (pEntityEntry->status & entity_ghost) && (pEntity->status & entity_limite) || pEntity->status & entity_outside || pEntityEntry->status & entity_outside))
					listAddNodeTail(pEntityEntry->entityview, pEntity);
			}
				
			if (pEntity->update != 0)
				continue;

			if (pEntity->entityid != pEntityEntry->entityid) {
				if (!(s->isBigWorld && (pEntity->status & entity_ghost) && (pEntityEntry->status & entity_limite) || pEntity->status & entity_outside || pEntityEntry->status & entity_outside))
					listAddNodeTail(pEntity->entityview, pEntityEntry);
			}
		}
	}
}

static void DestroyRtreeFun(void* value) {
	free(value);
}

void SudokuUpdate(void* pSudoku) {
	PSudoku s = (PSudoku)pSudoku;

	listIter* iter = listGetIterator(s->entitiesList, AL_START_HEAD);
	listNode* node;
	while ((node = listNext(iter)) != NULL) {
		PEntity pEntity = listNodeValue(node);

		if (pEntity->velocity == 0 || pEntity->update == 2 || pEntity->status & entity_outside)
			continue;

		struct Vector3 forward, nowPotion;
		transformForward(&pEntity->transform, &forward);
		float step = (GetCurrentSec() - pEntity->stamp) * pEntity->velocity;

		vector3Scale(&forward, &forward, step);
		vector3Add(&pEntity->transform.position, &forward, &nowPotion);

		if (nowPotion.x < s->begin.x || nowPotion.x > s->end.x || nowPotion.z < s->begin.z || nowPotion.z > s->end.z) {

			pEntity->status |= entity_outside;
			continue;
		}

		pEntity->oldGird = GirdId(s, &pEntity->nowPotion);

		pEntity->nowPotion = nowPotion;

		unsigned int girdid = GirdId(s, &pEntity->nowPotion);
		if (pEntity->oldGird == girdid) {
			pEntity->update = 0;
		}
		else {
			//����gird
			pEntity->update = 1;

			dictEntry* entry = dictFind(s->grid_entity, &pEntity->oldGird);
			list* girdlist = NULL;
			if (entry == NULL) {
				n_error("sudoku::Update error not find gird id %i", pEntity->oldGird);
				continue;
			}
			else {
				girdlist = dictGetVal(entry);
			}

			listPickNode(girdlist, pEntity->girdNode);

			if (listLength(girdlist) == 0) {
				dictDelete(s->grid_entity, &pEntity->oldGird);
			}

			entry = dictFind(s->grid_entity, &girdid);
			if (entry == NULL) {
				girdlist = listCreate();
				dictAddWithUint(s->grid_entity, girdid, girdlist);
			}
			else {
				girdlist = dictGetVal(entry);
			}

			listAddNodeHeadForNode(girdlist, pEntity->girdNode);
			pEntity->girdNode = listFirst(girdlist);
		}
	}
	listReleaseIterator(iter);


	iter = listGetIterator(s->entitiesList, AL_START_HEAD);
	node;
	while ((node = listNext(iter)) != NULL) {
		//������Χ����ÿ���û���listview
		PEntity pEntity = listNodeValue(node);
		if (pEntity->status & entity_outside && pEntity->outingStamp != 0)
			continue;

		unsigned int noteLimite = 0;

		if (pEntity->update == 1) {
			unsigned int gird = GirdId(s, &pEntity->nowPotion);
			enum SudokuDir sudokuDir = GirdDir(s, gird, pEntity->oldGird);

			switch (sudokuDir)
			{
			case up:
			{
				FillView(s, up, gird, pEntity);
				FillView(s, upleft, gird, pEntity);
				FillView(s, upright, gird, pEntity);
			}
			break;
			case down:
			{
				FillView(s, down, gird, pEntity);
				FillView(s, downleft, gird, pEntity);
				FillView(s, downright, gird, pEntity);
			}
			break;
			case left:
			{
				FillView(s, upleft, gird, pEntity);
				FillView(s, left, gird, pEntity);
				FillView(s, downleft, gird, pEntity);
			}
			break;
			case right:
			{
				FillView(s, upright, gird, pEntity);
				FillView(s, right, gird, pEntity);
				FillView(s, downright, gird, pEntity);
			}
			break;
			case upleft:
			{
				FillView(s, up, gird, pEntity);
				FillView(s, upleft, gird, pEntity);
				FillView(s, upright, gird, pEntity);
				FillView(s, left, gird, pEntity);
				FillView(s, downleft, gird, pEntity);
			}
			break;
			case upright:
			{
				FillView(s, up, gird, pEntity);
				FillView(s, upleft, gird, pEntity);
				FillView(s, upright, gird, pEntity);
				FillView(s, right, gird, pEntity);
				FillView(s, downright, gird, pEntity);
			}
			break;
			case downleft:
			{
				FillView(s, upleft, gird, pEntity);
				FillView(s, left, gird, pEntity);
				FillView(s, downleft, gird, pEntity);
				FillView(s, down, gird, pEntity);
				FillView(s, downright, gird, pEntity);
			}
			break;
			case downright:
			{
				FillView(s, upright, gird, pEntity);
				FillView(s, right, gird, pEntity);
				FillView(s, downright, gird, pEntity);
				FillView(s, down, gird, pEntity);
				FillView(s, downleft, gird, pEntity);
			}
			break;
			default:
				n_error("SudokuUpdate::step2 eid: %U, np: %i, op: %i", pEntity->entityid, gird, pEntity->oldGird);
			}

			//����߽������֪ͨ�û���½�����ռ�
			if (s->isBigWorld && GirdLimite(s, &pEntity->nowPotion)) {
				noteLimite = 1;
			}
			else if (s->isBigWorld && pEntity->status & entity_limite) {
				pEntity->status &= ~entity_limite;
			}
		}
		else if (pEntity->update == 2) {
			unsigned int gird = GirdId(s, &pEntity->nowPotion);

			FillView(s, up, gird, pEntity);
			FillView(s, down, gird, pEntity);
			FillView(s, left, gird, pEntity);
			FillView(s, right, gird, pEntity);
			FillView(s, upleft, gird, pEntity);
			FillView(s, upright, gird, pEntity);
			FillView(s, downleft, gird, pEntity);
			FillView(s, downright, gird, pEntity);
			FillView(s, dircentre, gird, pEntity);

			//����߽������֪ͨ�û���½�����ռ�
			if (s->isBigWorld && GirdLimite(s, &pEntity->nowPotion)) {
				noteLimite = 1;
				pEntity->status |= entity_limite;
			}
			else if(s->isBigWorld && pEntity->status & entity_limite){
				pEntity->status &= ~entity_limite;
				pEntity->status &= ~entity_ghost;
			}
		}

		if (noteLimite || pEntity->status & entity_outside) {

			if(pEntity->status & entity_outside)
				pEntity->outingStamp = GetCurrentSec();
			
			double rect[4] = { 0 };
			rect[0] = pEntity->nowPotion.x;
			rect[1] = pEntity->nowPotion.z;
			rect[2] = pEntity->nowPotion.x;
			rect[3] = pEntity->nowPotion.z;

			list* result = listCreate();
			rtree_search(s->prtree, rect, MultipleIter, result);

			mp_buf* pmp_buf = mp_buf_new();
			mp_encode_bytes(pmp_buf, "OnEntrySpace", strlen("OnEntrySpace"));

			listIter* iter = listGetIterator(result, AL_START_HEAD);
			listNode* node;
			while ((node = listNext(iter)) != NULL) {
				struct Iter* iter = (struct Iter*)listNodeValue(node);
				mp_encode_double(pmp_buf, u642double(*(unsigned long long*)iter->item));		
			}
			
			DockerSend(pEntity->entityid, pmp_buf->b, pmp_buf->len);
			mp_buf_free(pmp_buf);

			listSetFreeMethod(result, DestroyRtreeFun);
			listRelease(result);
		}
	}
	listReleaseIterator(iter);

	iter = listGetIterator(s->entitiesList, AL_START_HEAD);
	node;
	while ((node = listNext(iter)) != NULL) {
		PEntity pEntity = listNodeValue(node);
		pEntity->update = 0;

		if (listLength(pEntity->entityview) != 0) {
			mp_buf* pmp_buf = mp_buf_new();

			mp_encode_bytes(pmp_buf, "OnAddView", sizeof("OnAddView"));
			mp_encode_array(pmp_buf, listLength(pEntity->entityview));

			listIter* viewiter = listGetIterator(pEntity->entityview, AL_START_HEAD);
			listNode* viewnode;
			while ((viewnode = listNext(viewiter)) != NULL) {
				// ���Ͳ����listview���ͻ���
				PEntity pViewEntity = listNodeValue(viewnode);

				mp_encode_array(pmp_buf, 6);
				mp_encode_double(pmp_buf, u642double(pViewEntity->entityid));
				mp_encode_double(pmp_buf, pViewEntity->transform.position.x);
				mp_encode_double(pmp_buf, pViewEntity->transform.position.z);
				mp_encode_double(pmp_buf, pViewEntity->transform.rotation.y);
				mp_encode_double(pmp_buf, pViewEntity->velocity);
				mp_encode_int(pmp_buf, pViewEntity->stamp);

				listDelNode(pEntity->entityview, viewnode);
			}
			listReleaseIterator(viewiter);

			DockerSend(pEntity->entityid, pmp_buf->b, pmp_buf->len);
			mp_buf_free(pmp_buf);
		}

		//ɾ���Ѿ�����60���뿪�߽�Ķ���
		if (pEntity->status & entity_outside && pEntity->outingStamp && (GetCurrentSec() - pEntity->outingStamp) > OUTSIDESEC) {
			SudokuLeave(s, pEntity->entityid);

			mp_buf* pmp_buf = mp_buf_new();
			mp_encode_bytes(pmp_buf, "OnLeaveSpace", strlen("OnLeaveSpace"));
			mp_encode_double(pmp_buf, u642double(pEntity->entityid));

			DockerSend(pEntity->entityid, pmp_buf->b, pmp_buf->len);
			mp_buf_free(pmp_buf);
		}
	}
	listReleaseIterator(iter);
}

void PrintPoition(void* pSudoku) {
	PSudoku s = (PSudoku)pSudoku;

	listIter* iter = listGetIterator(s->entitiesList, AL_START_HEAD);
	listNode* node;
	while ((node = listNext(iter)) != NULL) {
		PEntity pEntity = listNodeValue(node);

		printf("%llu %f %f\n", pEntity->entityid, pEntity->nowPotion.x, pEntity->nowPotion.z);
	}
	listReleaseIterator(iter);
}


//����Ҫ���͹㲥
static int luaB_Update(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);

	SudokuUpdate(s);
	return 1;
}

static void freeValueCallback(void* privdata, void* val) {
	DICT_NOTUSED(privdata);
	listRelease(val);
}

static dictType UintDictType = {
	hashCallback,
	NULL,
	NULL,
	uintCompareCallback,
	memFreeCallback,
	freeValueCallback
};

void* SudokuCreate(struct Vector3 gird, struct Vector3 begin, struct Vector3 end, int isBigWorld) {
	PSudoku s = calloc(1, sizeof(Sudoku));

	s->entitiesList = listCreate();
	s->entitiesDict = dictCreate(DefaultLonglongPtr(), NULL);
	s->grid_entity = dictCreate(&UintDictType, NULL);

	s->gird = gird;
	s->begin = begin;
	s->end = end;

	s->end.x = s->end.x - 0.00001;
	s->end.z = s->end.z - 0.00001;

	s->linex = (unsigned int)ceil((s->end.x - s->begin.x) / s->gird.x);
	unsigned int dz = (unsigned int)floor((s->end.z - s->begin.z) / s->gird.z);
	s->maxid = s->linex + s->linex * dz;

	s->isBigWorld = isBigWorld;

	return s;
}

static int luaB_Create(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	struct Vector3 gird;
	struct Vector3 begin;
	struct Vector3 end;
	gird.x = luaL_checknumber(L, 1);
	gird.z = luaL_checknumber(L, 2);
	begin.x = luaL_checknumber(L, 3);
	begin.z = luaL_checknumber(L, 4);
	end.x = luaL_checknumber(L, 5);
	end.z = luaL_checknumber(L, 6);
	int isBigWorld = luaL_checkint(L, 7);

	lua_pushlightuserdata(L, SudokuCreate(gird, begin, end, isBigWorld));
	return 1;
}

static void DestroyFun(void* value) {
	PEntity pEntity = value;
	listRelease(pEntity->entityview);
	free(pEntity);
}

void SudokuDestory(void* pSudokus) {

	PSudoku s = (PSudoku)pSudokus;
	dictRelease(s->grid_entity);
	dictRelease(s->entitiesDict);

	listSetFreeMethod(s->entitiesList, DestroyFun);
	listRelease(s->entitiesList);

	free(s);
}

static int luaB_Destroy(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);

	SudokuDestory(s);
	return 1;
}

static int luaB_Poition(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");

	//position
	struct Vector3 position;
	position.x = luaL_checknumber(L, 1);
	position.y = 0;
	position.z = luaL_checknumber(L, 2);

	//rotation
	struct Vector3 angles;
	angles.x = luaL_checknumber(L, 3) * RADIANS_PER_DEGREE;
	angles.y = 0;
	angles.z = luaL_checknumber(L, 4) * RADIANS_PER_DEGREE;

	//velocity ÿ���ƶ����ٶ�
	float velocity = luaL_checknumber(L, 5);

	//stamp ��
	float stamp = luaL_checkinteger(L, 6);

	struct Quaternion quaternion;
	quatEulerAngles(&angles, &quaternion);

	struct Vector3 out;
	quatMultVector(&quaternion, &gForward, &out);

	vector3Scale(&out, &out, (GetCurrentSec() - stamp) * velocity);

	lua_pushnumber(L, out.x);
	lua_pushnumber(L, out.z);

	return 2;
}

static int luaB_Dist(lua_State* L) {

	struct Vector3 position1;
	position1.x = luaL_checknumber(L, 1);
	position1.y = 0;
	position1.z = luaL_checknumber(L, 2);

	//position
	struct Vector3 position;
	position.x = luaL_checknumber(L, 3);
	position.y = 0;
	position.z = luaL_checknumber(L, 4);

	//rotation
	struct Vector3 angles;
	angles.x = luaL_checknumber(L, 5) * RADIANS_PER_DEGREE;
	angles.y = 0;
	angles.z = luaL_checknumber(L, 6) * RADIANS_PER_DEGREE;

	//velocity ÿ���ƶ����ٶ�
	float velocity = luaL_checknumber(L, 7);

	//stamp ��
	float stamp = luaL_checkinteger(L, 8);

	struct Quaternion quaternion;
	quatEulerAngles(&angles, &quaternion);

	struct Vector3 out;
	quatMultVector(&quaternion, &gForward, &out);

	vector3Scale(&out, &out, (GetCurrentSec() - stamp) * velocity);

	lua_pushnumber(L, sqrtf(vector3DistSqrd(&position1, &out)));
	return 1;
}

static int luaB_Config(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);

	s->begin.x = luaL_checknumber(L, 2);
	s->begin.z = luaL_checknumber(L, 3);
	s->end.x = luaL_checknumber(L, 4);
	s->end.z = luaL_checknumber(L, 5);

	return 0;
}

static int luaB_Insert(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);

	unsigned long long id = luaL_tou64(L, 2);

	double rect[4] = { 0 };
	rect[0] = luaL_checknumber(L, 3);
	rect[1] = luaL_checknumber(L, 4);
	rect[2] = luaL_checknumber(L, 5);
	rect[3] = luaL_checknumber(L, 6);

	rtree_insert(s->prtree, rect, &id);
	return 0;
}

static int luaB_Alter(lua_State* L) {
	void* pVoid = LVMGetGlobleLightUserdata(L, "dockerHandle");
	PSudoku s = lua_touserdata(L, 1);

	unsigned long long id = luaL_tou64(L, 2);

	double rect[4] = { 0 };
	rect[0] = luaL_checknumber(L, 3);
	rect[1] = luaL_checknumber(L, 4);
	rect[2] = luaL_checknumber(L, 5);
	rect[3] = luaL_checknumber(L, 6);

	rtree_delete(s->prtree, rect, &id);
	rtree_insert(s->prtree, rect, &id);

	return 0;
}

static const luaL_Reg sudoku_funcs[] = {
	{"Create", luaB_Create},
	{"Destroy", luaB_Destroy},
	{"Update", luaB_Update},
	{"Move", luaB_Move},
	{"Leave", luaB_Leave},
	{"Entry", luaB_Entry},
	{"Poition", luaB_Poition},
	{"Dist", luaB_Dist},
	{"Config", luaB_Config},
	{"Insert", luaB_Insert},
	{"Alter", luaB_Alter},
	{NULL, NULL}
};

int LuaOpenSudoku(lua_State* L) {
	luaL_register(L, "sudokuapi", sudoku_funcs);
	return 1;
}