#include "cbase.h"
#include "Timer.h"
#include "filesystem.h"
#include "TimerTriggers.h"

#include "tier0/memdbgon.h"

extern IFileSystem *filesystem;

void CTimer::Start(int start)
{
    m_iStartTick = start;
    SetRunning(true);
    DispatchStateMessage();
}

void CTimer::OnTimeSubmitted(HTTPRequestCompleted_t *pCallback, bool bIOFailure)
{
    if (bIOFailure) return;
    DevLog("Recieved callback request!\n");
    uint32 size;
    steamapicontext->SteamHTTP()->GetHTTPResponseBodySize(pCallback->m_hRequest, &size);
    DevLog("Size of body: %u\n", size);
    uint8 *pData = new uint8[size];
    steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pCallback->m_hRequest, pData, size);
    DevLog("It is: %s\n", reinterpret_cast<char*>(pData));
    //MOM_TODO: Once the server updates this to contain more info, parse and do more with the response

    //Last but not least, free resources
    steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pCallback->m_hRequest);
}

void CTimer::PostTime()
{
    if (steamapicontext->SteamHTTP() && steamapicontext->SteamUser())
    {
        uint64 steamID = steamapicontext->SteamUser()->GetSteamID().ConvertToUint64();
        char steamIDString[21];
        Q_snprintf(steamIDString, 21, "%llu", steamID);
        const char* map = gpGlobals->mapname.ToCStr();
        int ticks = gpGlobals->tickcount - m_iStartTick;
        char ticksString[64];
        Q_snprintf(ticksString, 64, "%i", ticks);

        // TODO: make tickrate code crossplatform
#ifdef _WIN32
        float tickRate = TickSet::GetTickrate();
#else
        float tickRate = 0.015;
#endif

        //Build URL
        char webURL[512];
        Q_strcpy(webURL, "http://momentum-mod.org/postscore/");
        Q_strcat(webURL, steamIDString, 512);
        Q_strcat(webURL, "/", 512);
        Q_strcat(webURL, map, 512);
        Q_strcat(webURL, "/", 512);
        Q_strcat(webURL, ticksString, 512);
        Q_strcat(webURL, "/", 512);
        Q_strncat(webURL, tickRate == 0.01f ? "100" : "66", 512); // FIXME
        DevLog("Ticks sent to server: %i\n", ticks);

        //Build request
        HTTPRequestHandle handle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, webURL);
        SteamAPICall_t apiHandle;
        if (steamapicontext->SteamHTTP()->SendHTTPRequest(handle, &apiHandle))
        {
            OnTimeSubmittedCallback.Set(apiHandle, this, &CTimer::OnTimeSubmitted);
            DevLog("Successfully posted time!\n");
        }
        else
        {
            Warning("Failed to send HTTP Request to post scores online!\n");
            steamapicontext->SteamHTTP()->ReleaseHTTPRequest(handle);//GC
        }
    }
    else
    {
        Warning("Failed to post scores online: Cannot access STEAM HTTP or Steam User!\n");
    }
}

//Called upon map load, loads any and all times stored in the <mapname>.tim file
void CTimer::LoadLocalTimes(const char *szMapname)
{
    char timesFilePath[MAX_PATH];
    Q_strcpy(timesFilePath, c_mapDir);
    Q_strcat(timesFilePath, szMapname, MAX_PATH);
    Q_strncat(timesFilePath, c_timesExt, MAX_PATH);
    KeyValues *timesKV = new KeyValues(szMapname);
    if (timesKV->LoadFromFile(filesystem, timesFilePath, "MOD"))
    {
        for (KeyValues *kv = timesKV->GetFirstSubKey(); kv; kv = kv->GetNextKey())
        {
            Time t;
            t.ticks = Q_atoi(kv->GetName());
            t.tickrate = kv->GetFloat("rate");
            t.date = (time_t) kv->GetInt("date");
            localTimes.AddToTail(t);
        }
    }
    else
    {
        DevLog("Failed to load local times; no local file was able to be loaded!\n");
    }
    timesKV->deleteThis();
}

//Called every time a new time is achieved
void CTimer::SaveTime()
{
    const char *szMapName = gpGlobals->mapname.ToCStr();
    KeyValues *timesKV = new KeyValues(szMapName);
    int count = localTimes.Count();

    for (int i = 0; i < count; i++)
    {
        Time t = localTimes[i];
        char timeName[512];
        Q_snprintf(timeName, 512, "%i", t.ticks);
        KeyValues *pSubkey = new KeyValues(timeName);
        pSubkey->SetFloat("rate", t.tickrate);
        pSubkey->SetInt("date", t.date);
        timesKV->AddSubKey(pSubkey);
    }

    char file[MAX_PATH];
    Q_strcpy(file, c_mapDir);
    Q_strcat(file, szMapName, MAX_PATH);
    Q_strncat(file, c_timesExt, MAX_PATH);

    if (timesKV->SaveToFile(filesystem, file, "MOD", true))
        Log("Successfully saved new time!\n");

    timesKV->deleteThis();
}

void CTimer::Stop(bool endTrigger)
{
    if (endTrigger)
    {
        //Post time to leaderboards if they're online
        if (SteamAPI_IsSteamRunning())
            PostTime();

        //Save times locally too, regardless of SteamAPI condition
        Time t;
        t.ticks = gpGlobals->tickcount - m_iStartTick;
        t.tickrate = gpGlobals->interval_per_tick;
        time(&t.date);
        localTimes.AddToTail(t);

        SaveTime();
    }
    SetRunning(false);
    DispatchStateMessage();
}

void CTimer::OnMapEnd(const char *pMapName)
{
    SetCurrentCheckpointTrigger(NULL);
    SetStartTrigger(NULL);
    RemoveAllCheckpoints();
    localTimes.Purge();
    //MOM_TODO: onlineTimes.RemoveAll();
}

void CTimer::DispatchResetMessage()
{
    CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
    user.MakeReliable();
    UserMessageBegin(user, "Timer_Reset");
    MessageEnd();
}

void CTimer::DispatchStateMessage()
{
    CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
    user.MakeReliable();
    UserMessageBegin(user, "Timer_State");
    WRITE_BOOL(m_bIsRunning);
    WRITE_LONG(m_iStartTick);
    MessageEnd();
}

void CTimer::DispatchCheckpointMessage()
{
    CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
    user.MakeReliable();
    UserMessageBegin(user, "Timer_Checkpoint");
    MessageEnd();
}

bool CTimer::IsRunning()
{
    return m_bIsRunning;
}

void CTimer::SetRunning(bool running)
{
    m_bIsRunning = running;
}

CTriggerTimerStart *CTimer::GetStartTrigger()
{
    return m_pStartTrigger;
}

// Im not sure if this leaks memory or not
CTriggerCheckpoint *CTimer::GetCheckpointAt(int checkpointNumber)
{
    CBaseEntity* pEnt = gEntList.FindEntityByClassname(NULL, "trigger_timer_checkpoint");
    while (pEnt)
    {
        CTriggerCheckpoint* pTrigger = dynamic_cast<CTriggerCheckpoint*>(pEnt);
        if (pTrigger->GetCheckpointNumber() == checkpointNumber)
        {
            return pTrigger;
        }
        pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_timer_checkpoint");
    }
    return NULL;
}

CTriggerCheckpoint *CTimer::GetCurrentCheckpoint()
{
    return m_pCurrentCheckpoint;
}

void CTimer::SetStartTrigger(CTriggerTimerStart *pTrigger)
{
    m_pStartTrigger = pTrigger;
}

void CTimer::SetCurrentCheckpointTrigger(CTriggerCheckpoint *pTrigger)
{
    // Maybe find a better logic for this one?
    // It works, but it's not pretty
    if (m_pCurrentCheckpoint == NULL)
    {
        m_pCurrentCheckpoint = pTrigger;
    }
    else //pointer is not null
    {
        if (pTrigger != NULL)
        {
            // Is this the starting trigger?
            if (pTrigger->GetCheckpointNumber() == 0)
            {
                // Then set it, even if its index is lower than current's one
                m_pCurrentCheckpoint = pTrigger;
            }
            else if (pTrigger->GetCheckpointNumber() > m_pCurrentCheckpoint->GetCheckpointNumber())
            {
                // This is why we want to separate this if:
                DispatchCheckpointMessage();
                m_pCurrentCheckpoint = pTrigger;
            }
        }
        else
        {
            m_pCurrentCheckpoint = pTrigger;
        }
    }
}

void CTimer::CreateCheckpoint(CBasePlayer *pPlayer)
{
    if (!pPlayer) return;
    Checkpoint c;
    c.ang = pPlayer->GetAbsAngles();
    c.pos = pPlayer->GetAbsOrigin();
    c.vel = pPlayer->GetAbsVelocity();
    checkpoints.AddToTail(c);
    // MOM_TODO: Check what gametype we're in, so we can determine if we should stop the timer or not
    g_Timer.SetRunning(false);
    m_iCurrentStepCP++;
}

void CTimer::RemoveLastCheckpoint()
{
    if (checkpoints.IsEmpty()) return;
    checkpoints.Remove(m_iCurrentStepCP);
    m_iCurrentStepCP--;//If there's one element left, we still need to decrease currentStep to -1
}

void CTimer::TeleportToCP(CBasePlayer* cPlayer, int cpNum)
{
    if (checkpoints.IsEmpty() || !cPlayer) return;
    Checkpoint c = checkpoints[cpNum];
    cPlayer->Teleport(&c.pos, &c.ang, &c.vel);
}


// CTriggerOnehop
int CTimer::AddOnehopToListTail(CTriggerOnehop *pTrigger)
{
    return onehops.AddToTail(pTrigger);
}

// CTriggerOnehop
bool CTimer::RemoveOnehopFromList(CTriggerOnehop *pTrigger)
{
    return onehops.FindAndRemove(pTrigger);
}

// CTriggerOnehop
int CTimer::FindOnehopOnList(CTriggerOnehop *pTrigger)
{
    return onehops.Find(pTrigger);
}

// CTriggerOnehop
CTriggerOnehop *CTimer::FindOnehopOnList(int pIndexOnList)
{
    return onehops.Element(pIndexOnList);
}


class CTimerCommands
{
public:
    static void ResetToStart()
    {
        // MOM_TODO(fatalis):
        // if the ent no longer exists this will crash
        // should probably use a handle or something
        CBasePlayer* cPlayer = UTIL_GetLocalPlayer();
        if (cPlayer != NULL)
        {
            CTriggerTimerStart *start;
            if ((start = g_Timer.GetStartTrigger()) != NULL)
            {
                UTIL_SetOrigin(cPlayer, start->WorldSpaceCenter(), true);
                cPlayer->SetAbsVelocity(vec3_origin);
            }
        }
    }

    static void ResetToCheckpoint()
    {
        CTriggerCheckpoint *checkpoint;
        if ((checkpoint = g_Timer.GetCurrentCheckpoint()) != NULL)
        {
            UTIL_SetOrigin(UTIL_GetLocalPlayer(), checkpoint->WorldSpaceCenter(), true);
            UTIL_GetLocalPlayer()->SetAbsVelocity(vec3_origin);
        }
    }

    static void CPMenu(const CCommand &args)
    {
        if (g_Timer.IsRunning())
        {
            // MOM_TODO consider having a local timer running,
            //as people may want to time their routes they're using CP menu for
            // MOM_TODO consider KZ (lol)
            //g_Timer.SetRunning(false);
        }
        if (args.ArgC() > 0)
        {
            int sel = Q_atoi(args[1]);
            CBasePlayer* cPlayer = UTIL_GetLocalPlayer();
            switch (sel)
            {
            case 1://create a checkpoint
                g_Timer.CreateCheckpoint(cPlayer);
                break;

            case 2://load previous checkpoint
                g_Timer.TeleportToCP(cPlayer, g_Timer.GetCurrentCPMenuStep());
                break;

            case 3://cycle through checkpoints forwards (+1 % length)
                if (g_Timer.GetCPCount() > 0)
                {
                    g_Timer.SetCurrentCPMenuStep((g_Timer.GetCurrentCPMenuStep() + 1) % g_Timer.GetCPCount());
                    g_Timer.TeleportToCP(cPlayer, g_Timer.GetCurrentCPMenuStep());
                }
                break;

            case 4://cycle backwards through checkpoints
                if (g_Timer.GetCPCount() > 0)
                {
                    g_Timer.SetCurrentCPMenuStep(g_Timer.GetCurrentCPMenuStep() == 0 ? g_Timer.GetCPCount() - 1 : g_Timer.GetCurrentCPMenuStep() - 1);
                    g_Timer.TeleportToCP(cPlayer, g_Timer.GetCurrentCPMenuStep());
                }
                break;

            case 5://remove current checkpoint
                g_Timer.RemoveLastCheckpoint();
                break;

            case 6://remove every checkpoint
                g_Timer.RemoveAllCheckpoints();
                break;

            default:
                if (cPlayer != NULL)
                {
                    cPlayer->EmitSound("Momentum.UIMissingMenuSelection");
                }
                break;
            }
        }
    }
};

static ConCommand mom_reset_to_start("mom_restart", CTimerCommands::ResetToStart, "Restarts the run");
static ConCommand mom_reset_to_checkpoint("mom_reset", CTimerCommands::ResetToCheckpoint, "Teleports the player back to the last checkpoint");
static ConCommand mom_cpmenu("cpmenu", CTimerCommands::CPMenu, "", FCVAR_HIDDEN | FCVAR_SERVER_CAN_EXECUTE);

CTimer g_Timer;
