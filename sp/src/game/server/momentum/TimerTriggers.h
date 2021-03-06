#ifndef TIMERTRIGGERS_H
#define TIMERTRIGGERS_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "filters.h"

// CBaseMomentumTrigger
class CBaseMomentumTrigger : public CTriggerMultiple
{
	DECLARE_CLASS(CBaseMomentumTrigger, CTriggerMultiple);

public:
	virtual void Spawn();
};

// CTriggerTimerStop
class CTriggerTimerStop : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerTimerStop, CBaseMomentumTrigger);

public:
	void StartTouch(CBaseEntity*);
};

// CTriggerCheckpoint
class CTriggerCheckpoint : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerCheckpoint, CBaseMomentumTrigger);
	DECLARE_DATADESC();

public:
	void StartTouch(CBaseEntity*);
	int GetCheckpointNumber();
	void SetCheckpointNumber(int);

private:
	int m_iCheckpointNumber;
};

// CTriggerTimerStart
class CTriggerTimerStart : public CTriggerCheckpoint
{
    DECLARE_CLASS(CTriggerTimerStart, CTriggerCheckpoint);
	DECLARE_DATADESC();

public:
    void EndTouch(CBaseEntity*);
    void StartTouch(CBaseEntity*);
	void Spawn();
	// The start is always the first checkpoint: 0
    int GetCheckpointNumber() { return 0; }
	float GetMaxLeaveSpeed() { return m_fMaxLeaveSpeed; }
	void SetMaxLeaveSpeed(float pMaxSpeed);
	bool GetIsLimitingSpeed() { return HasSpawnFlags(SF_LIMIT_LEAVE_SPEED); }
	void SetIsLimitingSpeed(bool pIsLimitingSpeed);

private:
	// How fast can the player leave the start trigger?
	float m_fMaxLeaveSpeed = 280;
	// Limit max leave speed to m_fMaxLeaveSpeed?
	const int SF_LIMIT_LEAVE_SPEED = 0x2;

};

// CFilterCheckpoint
class CFilterCheckpoint : public CBaseFilter
{
    DECLARE_CLASS(CFilterCheckpoint, CBaseFilter);
    DECLARE_DATADESC();

public:
    bool PassesFilterImpl(CBaseEntity*, CBaseEntity*);

private:
    int m_iCheckpointNumber;

};

// CTriggerTeleportCheckpoint
class CTriggerTeleportCheckpoint : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerTeleportCheckpoint, CBaseMomentumTrigger);
	DECLARE_DATADESC();

public:
	void StartTouch(CBaseEntity*);
	int GetDestinationCheckpointNumber() { return m_iCheckpointNumber; }
	bool GetShouldStopPlayer() { return m_bResetVelocity; };
	// -1: Current checkpoint
	// Default: Checkpoint with pNewNumber index
	void SetDestinationCheckpointNumber(int);
	void SetDestinationCheckpointName(string_t);
	string_t GetDestinationCheckpointName() { return m_sLinkedTriggerName; };
	void SetShouldStopPlayer(bool);
	void Spawn();

private:
	// Where to teleport the player.
	// -1: Current checkpoint
	// Default: Checkpoint with that index
	int m_iCheckpointNumber;
	// Should the player be stopped after teleport?
	bool m_bResetVelocity = false;
	// Linked Trigger (If desired trigger is not dynamic)
	CBaseMomentumTrigger *m_eLinkedTrigger;
	// Name of the linked trigger (We search for the one with this name if it's set)
	// If it's not set, m_iCheckpointNumber is used
	string_t m_sLinkedTriggerName;

protected:
	// Are we using the linked entity variable?
	bool m_bUsingLinked = false;

};

// CTriggerOnehop
class CTriggerOnehop : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerOnehop, CBaseMomentumTrigger);
	DECLARE_DATADESC();

public:
	void StartTouch(CBaseEntity*);
	int GetDestinationIndex() { return m_iDestinationCheckpointNumber; }
	bool GetShouldStopPlayer() { return m_bResetVelocity; }
	float GetHoldTeleportTime() { return m_fMaxHoldSeconds; }
	void SetDestinationIndex(int pNewIndex);
	void SetDestinationName(string_t);
	string_t GetDestinationName() { return m_sLinkedTriggerName; };
	void SetShouldStopPlayer(bool pShouldStop);
	void SetHoldTeleportTime(float pHoldTime);
    void Think();
    void HandleTeleport(CBaseEntity*);
	void Spawn();

private:
	// Should the player be stopped after teleport?
	bool m_bResetVelocity = true;
    // The time that the player initally touched the trigger
    float m_fStartTouchedTime = 0;
	// Seconds to hold before activating the teleport
	float m_fMaxHoldSeconds = 1;
	// Where to teleport the player if it becomes active
	int m_iDestinationCheckpointNumber = -1;
    // Reset hop state if player hops onto another different onehop
    const int SF_TELEPORT_RESET_ONEHOP = 0x2;	
	// Linked Trigger (If desired trigger is not dynamic)
	CBaseMomentumTrigger *m_eLinkedTrigger;
	// Name of the linked trigger (We search for the one with this name if it's set)
	// If it's not set, m_iDestinationCheckpointNumber is used
	string_t m_sLinkedTriggerName;

protected:
	// Are we using the linked entity variable?
	bool m_bUsingLinked = false;

};

// CTriggerResetOnehop
class CTriggerResetOnehop : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerResetOnehop, CBaseMomentumTrigger);

public:
	void StartTouch(CBaseEntity*);

};

// CTriggerMultihop
class CTriggerMultihop : public CBaseMomentumTrigger
{
	DECLARE_CLASS(CTriggerMultihop, CBaseMomentumTrigger);
	DECLARE_DATADESC();

public:
	void StartTouch(CBaseEntity*);
	void EndTouch(CBaseEntity*);
	int GetDestinationIndex() { return m_iDestinationCheckpointNumber; }
	bool GetShouldStopPlayer() { return m_bResetVelocity; }
	float GetHoldTeleportTime() { return m_fMaxHoldSeconds; }
	void SetDestinationIndex(int pNewIndex);
	void SetDestinationName(string_t);
	string_t GetDestinationName() { return m_sLinkedTriggerName; };
	void SetShouldStopPlayer(bool pShouldStop);
	void SetHoldTeleportTime(float pHoldTime);
	void Think();
	void HandleTeleport(CBaseEntity*);
	void Spawn();

private:
	// Should the player be stopped after teleport?
	bool m_bResetVelocity = true;
	// The time that the player initally touched the trigger. -1 if not checking for teleport
	float m_fStartTouchedTime = 0;
	// Seconds to hold before activating the teleport
	float m_fMaxHoldSeconds = 1;
	// Where to teleport the player if it becomes active.
	int m_iDestinationCheckpointNumber = -1;
	// Linked Trigger (If desired trigger is not dynamic)
	CBaseMomentumTrigger *m_eLinkedTrigger;
	// Name of the linked trigger (We search for the one with this name if it's set)
	// If it's not set, m_iDestinationCheckpointNumber is used
	string_t m_sLinkedTriggerName;

protected:
	// Are we using the linked entity variable?
	bool m_bUsingLinked = false;

};

// CTriggerUserInput
class CTriggerUserInput : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerUserInput, CBaseMomentumTrigger);
    DECLARE_DATADESC();
public:
    enum key { forward, back, moveleft, moveright };
    key m_eKey;

    void Think();
    void Spawn();

    COutputEvent m_OnKeyPressed;

private:
    int m_ButtonRep;
    
};

#endif // TIMERTRIGGERS_H
