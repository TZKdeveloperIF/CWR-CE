#pragma once

#include <Poseidon/Core/Types.hpp>
#include <Poseidon/AI/Path/AITypes.hpp>
#include <Poseidon/World/Entities/Vehicles/Transport.hpp>
#include <Poseidon/IO/Serialization/ParamArchive.hpp>
#include <Poseidon/Audio/Speaker.hpp>

#include <Poseidon/Foundation/Time/Time.hpp>

#include <Poseidon/World/Scene/Camera/CamEffects.hpp>
#include <Poseidon/Game/TitEffects.hpp>

#include <Poseidon/AI/Path/ArcadeWaypoint.hpp>

#include <Poseidon/Core/FSM/Fsm.hpp>

#include <Poseidon/Game/Scripting/Scripts.hpp>

#include <time.h>

extern int MaxGroups;

namespace Poseidon
{

struct ExperienceDestroyInfo
{
	float maxCost;
	float exp;
};

extern AutoArray<float> ExperienceTable;
extern AutoArray<ExperienceDestroyInfo> ExperienceDestroyTable;

//! Experience base for destroying a unit of the given cost.
/*! Returns the matching ExperienceDestroyTable bucket: the first entry whose
maxCost the cost fits under, or the final catch-all entry for costs above the last
sized bucket. Returns 0 only for an empty table. */
float ExperienceForDestroyedCost(float cost);

extern float ExperienceDestroyEnemy;
extern float ExperienceDestroyFriendly;
extern float ExperienceDestroyCivilian;

extern float ExperienceRenegadeLimit;

extern float ExperienceKilled;

// for subordinate soldier only
extern float ExperienceCommandCompleted;
extern float ExperienceCommandFailed;
extern float ExperienceFollowMe;

// for leadership only
extern float ExperienceDestroyYourUnit;
extern float ExperienceMissionCompleted;
extern float ExperienceMissionFailed;

struct SynchronizedGroup
{
	OLink<AIGroup> group;
	bool active;

	LSError Serialize(ParamArchive &ar);
};

struct SynchronizedSensor
{
	OLink<Vehicle> sensor;
	bool active;

	LSError Serialize(ParamArchive &ar);
};

struct SynchronizedItem
{
	AutoArray<SynchronizedGroup> groups;
	AutoArray<SynchronizedSensor> sensors;

	void Add(AIGroup *grp);
	void Add(Vehicle *sensor);

	void SetActive(AIGroup *grp, bool active = true);
	void SetActive(Vehicle *sensor, bool active = true);

	bool IsActive(AIGroup *grp = nullptr);

	LSError Serialize(ParamArchive &ar);
};

extern OLinkArray<EntityAI> vehiclesMap;
extern OLinkArray<Vehicle> sensorsMap;
class GblMarkerArrayWithHash
{
private:
	struct RStrIHasher
	{
		unsigned int operator()(const RString& str) const
		{
			return CalculateStringHashValueCI(str);
		}
	};
	struct RStrIEqual
	{
		bool operator()(const RString& lhs, const RString& rhs) const
		{
			return stricmp(lhs.Data(), rhs.Data()) == 0;
		}
	};
	AutoArray<ArcadeMarkerInfo> m_markersArr;
	std::unordered_map<RString, int, RStrIHasher, RStrIEqual> m_name2IdxMap;
public:
	// Transmit to AutoArray
	int Add(const ArcadeMarkerInfo& src)
	{
		const int insertedIdx = m_markersArr.Add(src);
		// It's acceptable in origin OFP that meets same name markers when reading mission.sqm
		// thus src.name can exist in m_name2IdxMap on adding new marker
		m_name2IdxMap.insert(std::make_pair(src.name, insertedIdx)); // can fail
		return insertedIdx;
	}
	int Add() = delete; // not allow Add default object first and then rename it
	int Size() const
	{
		return m_markersArr.Size();
	}
	void Clear()
	{
		m_markersArr.Clear();
		m_name2IdxMap.clear();
	}
	void Delete(int index)
	{
		// Note: Markers are created in three distinct ways. User-created markers (via the map) are shared across the network, carry a "_user_defined" prefix, and possess a unique ID, thus they never duplicate. Script-created markers (via createMarker) are local and have a duplicate-name check. Markers defined in mission.sqm may have duplicate names.
		// Since duplicates are possible, when deleting a marker by name, the deletion logic must advance its search index to the next marker with the same name if one exists.
		// For markers with the "_user_defined" prefix, uniqueness can usually be assumed. However, this assumption breaks if script-created or mission.sqm markers also employ that prefix, although such practices are rarely encountered.

		RString deletedMarkerName = m_markersArr[index].name;
		const char* userDefined = "_user_defined";
		const bool isUserDefMarker = strnicmp(deletedMarkerName, userDefined, strlen(userDefined)) == 0;
		const int oldSize = m_markersArr.Size();
		int newIdx = oldSize;

		m_markersArr.Delete(index);

		if (!isUserDefMarker)
		{
			// origin OFP will only delete first marker matches input name
			// search whether there exists markers using same name and update index value
			for (int i = index, n = m_markersArr.Size(); i < n; ++i)
			{
				if (stricmp(deletedMarkerName, m_markersArr[i].name) == 0)
				{
					newIdx = i;
					break;
				}
			}
		}
		for (auto it = m_name2IdxMap.begin(); it != m_name2IdxMap.end(); )
		{
			if (it->second < index)
			{
				++it;
			}
			else if (it->second == index)
			{
				if (newIdx == oldSize)	// no another marker whose name is deletedMarkerName
				{
					it = m_name2IdxMap.erase(it);
				}
				else					// still have marker whose name is deletedMarkerName
				{
					it->second = newIdx;
					++it;
				}
			}
			else // it->second > index
			{
				--it->second;
				++it;
			}
		}
	}
	ArcadeMarkerInfo& operator[](int i)
	{
		return m_markersArr[i];
	}
	const ArcadeMarkerInfo& operator[](int i) const
	{
		return m_markersArr[i];
	}

	// Search
	static bool ValidIdx(int index) { return index >= 0; }
	int FindIdx(const RString& name) const
	{
		auto it = m_name2IdxMap.find(name);
		if (it != m_name2IdxMap.cend())
			return it->second;
		return -1;
	}
	ArcadeMarkerInfo* Find(const RString& name)
	{
		const int idx = FindIdx(name);
		if (!ValidIdx(idx))
			return nullptr;
		return &m_markersArr[idx];
	}
	const ArcadeMarkerInfo* Find(const RString& name) const
	{
		const int idx = FindIdx(name);
		if (!ValidIdx(idx))
			return nullptr;
		return &m_markersArr[idx];
	}

	// For serialize
	AutoArray<ArcadeMarkerInfo>& ArrayForSerialize()
	{
		return m_markersArr;
	}
	void OnSerialize(const int oldSize)
	{
		for (int i = oldSize, c = m_markersArr.Size(); i < c; ++i)
		{
			m_name2IdxMap.insert(std::make_pair(m_markersArr[i].name, i)); // can fail
		}
	}
};
extern GblMarkerArrayWithHash markersMap;
extern AutoArray<SynchronizedItem> synchronized;

template <class Task, class ContextType>
class AbstractAIMachine
{
protected:
	struct Item
	{
		SRef<FSM> _fsm;
		Ref<Task> _task;
		LSError Serialize(ParamArchive &ar)
		{
			PARAM_CHECK(ar.Serialize("Task", _task, 1))
			if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
				_fsm = AbstractAIMachine<Task, ContextType>::CreateFSM(_task->GetType());
			PARAM_CHECK(ar.Serialize("FSM", *_fsm, 1))
			return LSOK;
		}
	};
	AutoArray<Item> _stack;
	SRef<FSM> _noTaskFSM;
public:
	AbstractAIMachine(ContextType *context = nullptr)
	{
		_noTaskFSM = CreateFSM(0);
		if (context)
		{
			context->_task = nullptr;
			context->_fsm = _noTaskFSM;
		}
		_noTaskFSM->Enter(context);
		_noTaskFSM->SetState(0, context);
	}
	~AbstractAIMachine() {}

	LSError Serialize(ParamArchive &ar);

	static FSM *CreateFSM(int taskType);
	void PushTask(Task &task,ContextType *context);									// add task to top of stack and make it current
	void EnqueueTask(Task &task,ContextType *context);							// add task to bottom of stack
	void PopTask(ContextType *context, bool doRefresh = true);			// remove task from top of stack
	void Delete(int i, ContextType *context, bool doRefresh = true);// remove task from top of stack
	void Clear(ContextType *context = nullptr);												// remove all tasks from stack
	bool Update(ContextType *context);
	void UpdateAndRefresh(ContextType *context);
	Item *GetCurrent()
	{int n = _stack.Size(); return n == 0 ? nullptr : &_stack[n - 1];}
	const Item *GetCurrent() const
	{int n = _stack.Size(); return n == 0 ? nullptr : &_stack[n - 1];}

	virtual void OnTaskCreated(int index, Task &task) {};
	virtual void OnTaskDeleted(int index, Task &task) {};
protected:
	void Enter(ContextType *context);
	void Exit(ContextType *context);
};

template <class Task, class ContextType>
LSError AbstractAIMachine<Task, ContextType>::Serialize(ParamArchive &ar)
{
	if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
		_noTaskFSM = CreateFSM(0);
	PARAM_CHECK(ar.Serialize("NoTaskFSM", *_noTaskFSM, 1))
	PARAM_CHECK(ar.Serialize("Stack", _stack, 1))
	return LSOK;
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::PushTask(Task &task, ContextType *context)
{
	AI_ERROR(task.GetType() > 0);	// Task 0 is used when no task in stack
	if (task.GetType() <= 0)
		return;
	Exit(context);
	int index = _stack.Add();
	AI_ERROR(GetCurrent());
	_stack[index]._task = new Task(task);
	_stack[index]._fsm = CreateFSM(task.GetType());
	OnTaskCreated(index, *GetCurrent()->_task);
	context->_task = GetCurrent()->_task;
	context->_fsm = GetCurrent()->_fsm;
	GetCurrent()->_fsm->Enter(context);
	GetCurrent()->_fsm->SetState(0, context);	// State 0 is starting state
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::EnqueueTask(Task &task, ContextType *context)
{
	AI_ERROR(task.GetType() > 0);	// Task 0 is used when no task in stack
	if (task.GetType() <= 0)
		return;
	if (_stack.Size() <= 0)
	{
		PushTask(task, context);
		return;
	}
	_stack.Insert(0);
	_stack[0]._task = new Task(task);
	_stack[0]._fsm = CreateFSM(task.GetType());
	OnTaskCreated(0, *_stack[0]._task);
	_stack[0]._fsm->SetState(0); // State 0 is starting state
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::PopTask(ContextType *context, bool doRefresh)
{
	int n = _stack.Size() - 1;
	if (n < 0)
	{
		return;
	}

	Exit(context);
	OnTaskDeleted(n, *_stack[n]._task);
	_stack.Delete(n);

	Enter(context);
	if (doRefresh) UpdateAndRefresh(context);
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::Delete(int i,ContextType *context, bool doRefresh)
{
	if (GetCurrent())
	{
		context->_task = GetCurrent()->_task;
		context->_fsm = GetCurrent()->_fsm;
	}
	else
	{
		context->_task = nullptr;
		context->_fsm = _noTaskFSM;
	}

	if( i==_stack.Size()-1 ) PopTask(context, doRefresh);
	else
	{
		OnTaskDeleted(i, *_stack[i]._task);
		_stack.Delete(i);
	}
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::Clear(ContextType *context)
{
	if (_stack.Size() == 0)
	{
		return;
	}

	Exit(context);
	for (int i=_stack.Size()-1; i>=0; i--)
		OnTaskDeleted(i, *_stack[i]._task);
	_stack.Clear();

	if (context)
	{
		Enter(context);
		UpdateAndRefresh(context);
	}
}

template <class Task, class ContextType>
bool AbstractAIMachine<Task, ContextType>::Update(ContextType *context)
{
	if (GetCurrent())
	{
		context->_task = GetCurrent()->_task;
		context->_fsm = GetCurrent()->_fsm;
		if (GetCurrent()->_fsm->Update(context))
		{
			PopTask(context);
			return true;
		}
	}
	else
	{
		context->_task = nullptr;
		context->_fsm = _noTaskFSM;
		_noTaskFSM->Update(context);
	}
	return false;
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::UpdateAndRefresh(ContextType *context)
{
	if (GetCurrent())
	{
		context->_task = GetCurrent()->_task;
		context->_fsm = GetCurrent()->_fsm;
	}
	else
	{
		context->_task = nullptr;
		context->_fsm = _noTaskFSM;
	}
	context->_fsm->Refresh(context);
	Update(context);
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::Enter(ContextType *context)
{
	if (GetCurrent())
	{
		context->_task = GetCurrent()->_task;
		context->_fsm = GetCurrent()->_fsm;
	}
	else
	{
		context->_task = nullptr;
		context->_fsm = _noTaskFSM;
	}
	context->_fsm->Enter(context);
}

template <class Task, class ContextType>
void AbstractAIMachine<Task, ContextType>::Exit(ContextType *context)
{
	if (GetCurrent())
	{
		context->_task = GetCurrent()->_task;
		context->_fsm = GetCurrent()->_fsm;
	}
	else
	{
		context->_task = nullptr;
		context->_fsm = _noTaskFSM;
	}
	context->_fsm->Exit(context);
}

class RoadLink;

struct BuildingLockInfo
{
	OLink<Object> house;
	int index;
};

class AILocker : public RefCount
{
	AutoArray< InitPtr<LockField> > _fields;	// locked fields
	AutoArray< InitPtr<RoadLink> > _roads;	// locked roads
	AutoArray<BuildingLockInfo> _buildings;	// locked building positions
public:
	AILocker();
	~AILocker() override;

	void LockPosition( Vector3Val pos, float radius, bool soldier, float size);
	void UnlockPosition( Vector3Val pos, float radius, bool soldier);

	void LockPositionMan( Vector3Val pos, float radius);
	void UnlockPositionMan( Vector3Val pos, float radius);

protected:
	// implementation
	void LockItem(int x, int z, bool soldier);
	void UnlockItem(int x, int z, bool soldier);
};

// Facade includes — split at AIUnit / AIGroup / AICenter seams

}  // namespace Poseidon
