#include "cbase.h"
#include "Timer.h"
#include "TimerTriggers.h"
#include "movevars_shared.h"

#include "tier0/memdbgon.h"

// CBaseMomentumTrigger
void CBaseMomentumTrigger::Spawn()
{
	BaseClass::Spawn();
	// temporary
	m_debugOverlays |= (OVERLAY_BBOX_BIT | OVERLAY_TEXT_BIT);
}


// CTriggerTimerStart
void CTriggerTimerStart::EndTouch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		sv_maxvelocity.SetValue(CBaseMomentumTrigger::PreMaxVel);
		g_Timer.Start(gpGlobals->tickcount);
		g_Timer.SetStartTrigger(this);
	}
	BaseClass::EndTouch(pOther);
}

void CTriggerTimerStart::StartTouch(CBaseEntity *pOther) {
	if (pOther->IsPlayer())
	{
		CBaseMomentumTrigger::PreMaxVel = sv_maxvelocity.GetInt();
		sv_maxvelocity.SetValue(260);
		if (g_Timer.IsRunning())
		{
			g_Timer.Stop();
			g_Timer.DispatchResetMessage();
		}
	}
	BaseClass::StartTouch(pOther);
}

LINK_ENTITY_TO_CLASS(trigger_timer_start, CTriggerTimerStart);


// CTriggerTimerStop
void CTriggerTimerStop::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if (pOther->IsPlayer())
		g_Timer.Stop();
}

LINK_ENTITY_TO_CLASS(trigger_timer_stop, CTriggerTimerStop);


// CTriggerCheckpoint
void CTriggerCheckpoint::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if (pOther->IsPlayer())
		g_Timer.SetCurrentCheckpoint(this);
}

int CTriggerCheckpoint::GetCheckpointNumber()
{
	return m_iCheckpointNumber;
}

LINK_ENTITY_TO_CLASS(trigger_checkpoint, CTriggerCheckpoint);

BEGIN_DATADESC(CTriggerCheckpoint)
DEFINE_KEYFIELD(m_iCheckpointNumber, FIELD_INTEGER, "number")
END_DATADESC()


static void TestCreateTriggerStart(void)
{
	CTriggerTimerStart *pTrigger = (CTriggerTimerStart *)CreateEntityByName("trigger_timer_start");
	if (pTrigger)
	{
		pTrigger->Spawn();
		pTrigger->SetAbsOrigin(UTIL_GetLocalPlayer()->GetAbsOrigin());
		pTrigger->SetSize(Vector(-256, -256, -256), Vector(256, 256, 256));
		pTrigger->SetSolid(SOLID_BBOX);
		pTrigger->SetName(MAKE_STRING("Start Trigger"));
		// now use mom_reset_to_start
	}
}

//Commented until further test
/*static void TestCreateTriggerStop(void)
{
	CTriggerTimerStop *pTrigger = (CTriggerTimerStop *)CreateEntityByName("trigger_timer_stop");
	if (pTrigger)
	{
		pTrigger->Spawn();
		pTrigger->SetAbsOrigin(UTIL_GetLocalPlayer()->GetAbsOrigin());
		pTrigger->SetSize(Vector(-256, -256, -256), Vector(256, 256, 256));
		pTrigger->SetSolid(SOLID_BBOX);
		pTrigger->SetName(MAKE_STRING("Stop Trigger"));
		// now use mom_reset_to_start
	}
}
*/
static ConCommand mom_createstart("mom_createstart", TestCreateTriggerStart, "Create StartTrigger test");
//static ConCommand mom_createstop("mom_createstop", TestCreateTriggerStop, "Create StopTrigger test");
