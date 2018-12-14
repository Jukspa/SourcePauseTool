#include "stdafx.hpp"

#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\patterns.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include "EngineDLL.hpp"
#include "..\scripts\srctas_reader.hpp"

using std::uintptr_t;
using std::size_t;

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer()
{
	return engineDLL.HOOKED_SV_ActivateServer_Func();
}

void __fastcall EngineDLL::HOOKED_FinishRestore(void* thisptr, int edx)
{
	return engineDLL.HOOKED_FinishRestore_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	return engineDLL.HOOKED_SetPaused_Func(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame(float time)
{
	return engineDLL.HOOKED__Host_RunFrame_Func(time);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick)
{
	return engineDLL.HOOKED__Host_RunFrame_Input_Func(accumulated_extra_samples, bFinalTick);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server(int bFinalTick)
{
	return engineDLL.HOOKED__Host_RunFrame_Server_Func(bFinalTick);
}

void __cdecl EngineDLL::HOOKED_Host_AccumulateTime(float dt)
{
	engineDLL.HOOKED_Host_AccumulateTime_Func(dt);
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute()
{
	return engineDLL.HOOKED_Cbuf_Execute_Func();
}

void * __cdecl EngineDLL::HOOKED_Cmd_ExecuteCommand(CCommand & command, int src, int nClientSlot)
{
	return engineDLL.HOOKED_Cmd_ExecuteCommand_Func(command, src, nClientSlot);
}

void EngineDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	uintptr_t pSpawnPlayer = NULL,
		pSV_ActivateServer = NULL,
		pFinishRestore = NULL,
		pSetPaused = NULL,
		pMiddleOfSV_InitGameDLL = NULL,
		pRecord = NULL,
		pHost_AccumulateTime = NULL,
		pMiddle_Of_Host_AccumulateTime = NULL,
		p_Host_RunFrame = NULL,
		pCmd_ExecuteCommand = NULL;

	auto fActivateServer = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsSV_ActivateServer, &pSV_ActivateServer);
	auto fFinishRestore = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsFinishRestore, &pFinishRestore);
	auto fSetPaused = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsSetPaused, &pSetPaused);
	auto fMiddleOfSV_InitGameDLL = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsMiddleOfSV_InitGameDLL, &pMiddleOfSV_InitGameDLL);
	auto fRecord = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsRecord, &pRecord);
	auto fHost_AccumulateTime = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsHost_AccumulateTime, &pHost_AccumulateTime);
	auto f_Host_RunFrame = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptns_Host_RunFrame, &p_Host_RunFrame);
	auto fCmd_ExecuteCommand = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsCmd_ExecuteCommand, &pCmd_ExecuteCommand);

	// m_bLoadgame and pGameServer (&sv)
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsSpawnPlayer, &pSpawnPlayer);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		EngineDevMsg("Found SpawnPlayer at %p (using the build %s pattern).\n", pSpawnPlayer, Patterns::ptnsSpawnPlayer[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 5));
			pGameServer = (*(void **)(pSpawnPlayer + 18));
			break;

		case 1:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;

		case 2: // 4104 is the same as 5135 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 5));
			pGameServer = (*(void **)(pSpawnPlayer + 18));
			break;

		case 3: // 2257546 is the same as 5339 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;

		case 4:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 26));
			//pGameServer = (*(void **)(pSpawnPlayer + 21)); - We get this one from SV_ActivateServer in OE.
			break;

		case 5: // 6879 is the same as 5339 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;
		}

		EngineDevMsg("m_bLoadGame is situated at %p.\n", pM_bLoadgame);

#if !defined( OE )
		EngineDevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		EngineDevWarning("Could not find SpawnPlayer!\n");
		EngineWarning("y_spt_pause 2 has no effect.\n");
	}

	// SV_ActivateServer
	ptnNumber = fActivateServer.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_SV_ActivateServer = (_SV_ActivateServer)pSV_ActivateServer;
		EngineDevMsg("Found SV_ActivateServer at %p (using the build %s pattern).\n", pSV_ActivateServer, Patterns::ptnsSV_ActivateServer[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 3:
			pGameServer = (*(void **)(pSV_ActivateServer + 223));
			break;
		}

#if defined( OE )
		EngineDevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		EngineDevWarning("Could not find SV_ActivateServer!\n");
		EngineWarning("y_spt_pause 2 has no effect.\n");
		EngineWarning("_y_spt_afterframes_reset_on_server_activate has no effect.\n");
	}

	// FinishRestore
	ptnNumber = fFinishRestore.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_FinishRestore = (_FinishRestore)pFinishRestore;
		EngineDevMsg("Found FinishRestore at %p (using the build %s pattern).\n", pFinishRestore, Patterns::ptnsFinishRestore[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("Could not find FinishRestore!\n");
		EngineWarning("y_spt_pause 1 has no effect.\n");
	}

	// SetPaused
	ptnNumber = fSetPaused.get();
	if (pSetPaused)
	{
		ORIG_SetPaused = (_SetPaused)pSetPaused;
		EngineDevMsg("Found SetPaused at %p (using the build %s pattern).\n", pSetPaused, Patterns::ptnsSetPaused[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("Could not find SetPaused!\n");
		EngineWarning("y_spt_pause has no effect.\n");
	}

	// interval_per_tick
	ptnNumber = fMiddleOfSV_InitGameDLL.get();
	if (pMiddleOfSV_InitGameDLL)
	{
		EngineDevMsg("Found the interval_per_tick pattern at %p (using the build %s pattern).\n", pMiddleOfSV_InitGameDLL, Patterns::ptnsMiddleOfSV_InitGameDLL[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			pIntervalPerTick = *reinterpret_cast<float**>(pMiddleOfSV_InitGameDLL + 18);
			break;

		case 1:
			pIntervalPerTick = *reinterpret_cast<float**>(pMiddleOfSV_InitGameDLL + 16);
			break;
		}
		
		EngineDevMsg("Found interval_per_tick at %p.\n", pIntervalPerTick);
	}
	else
	{
		EngineDevWarning("Could not find the interval_per_tick pattern!\n");
		EngineWarning("_y_spt_tickrate has no effect.\n");
	}

	ptnNumber = fRecord.get();
	if (pRecord)
	{
		pDemoplayer = *reinterpret_cast<void***>(pRecord + 132);

		EngineDevMsg("Found record at %p (using the build %s pattern).\n", pRecord, Patterns::ptnsRecord[ptnNumber].build.c_str());
		EngineDevMsg("Found demoplayer at %p.\n", pDemoplayer);
	}
	else
	{
		EngineDevWarning("Could not find record!\n");
		EngineWarning("y_spt_pause_demo_on_tick is not available.\n");
	}

	ptnNumber = fHost_AccumulateTime.get();
	if (pHost_AccumulateTime)
	{
		pHost_Realtime = *reinterpret_cast<float**>(pHost_AccumulateTime + 5);
		ORIG_Host_AccumulateTime = (_Host_AccumulateTime)pHost_AccumulateTime;

		EngineDevMsg("Found Host_AccumulateTime at %p (using the build %s pattern).\n", pHost_AccumulateTime, Patterns::ptnsHost_AccumulateTime[ptnNumber].build.c_str());
		EngineDevMsg("Found realtime at %p.\n", pHost_Realtime);
	}
	else
	{
		EngineDevWarning("Could not find record!\n");
		EngineWarning("y_spt_pause_demo_on_tick is not available.\n");
	}

	ptnNumber = f_Host_RunFrame.get();
	if (p_Host_RunFrame)
	{
		pHost_Frametime = *reinterpret_cast<float**>(p_Host_RunFrame + 227);
		pM_nSignonState = *reinterpret_cast<int**>(p_Host_RunFrame + 438);

		EngineDevMsg("Found _Host_RunFrame at %p (using the build %s pattern).\n", p_Host_RunFrame, Patterns::ptns_Host_RunFrame[ptnNumber].build.c_str());
		EngineDevMsg("Found host_Frametime at %p.\n", pHost_Frametime);
	}
	else
	{
		EngineDevWarning("Could not find _Host_RunFrame!\n");
	}

	ptnNumber = fCmd_ExecuteCommand.get();
	if (pCmd_ExecuteCommand)
	{
		ORIG_Cmd_ExecuteCommand = (_Cmd_ExecuteCommand)(pCmd_ExecuteCommand);

		EngineDevMsg("Found Cmd_ExecuteCommand at %p (using the build %s pattern).\n", pCmd_ExecuteCommand, Patterns::ptnsCmd_ExecuteCommand[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("Could not find Cmd_ExecuteCommand!\n");
	}


	DetoursUtils::AttachDetours(moduleName, {
		{ (PVOID *)(&ORIG_SV_ActivateServer), HOOKED_SV_ActivateServer },
		{ (PVOID *)(&ORIG_FinishRestore), HOOKED_FinishRestore },
		{ (PVOID *)(&ORIG_SetPaused), HOOKED_SetPaused },
		{ (PVOID *)(&ORIG__Host_RunFrame), HOOKED__Host_RunFrame },
		{ (PVOID *)(&ORIG__Host_RunFrame_Input), HOOKED__Host_RunFrame_Input },
		{ (PVOID *)(&ORIG__Host_RunFrame_Server), HOOKED__Host_RunFrame_Server },
		{ (PVOID *)(&ORIG_Host_AccumulateTime), HOOKED_Host_AccumulateTime },
		{ (PVOID *)(&ORIG_Cbuf_Execute), HOOKED_Cbuf_Execute },
		{ (PVOID *)(&ORIG_Cmd_ExecuteCommand), HOOKED_Cmd_ExecuteCommand }
	});
}

void EngineDLL::Unhook()
{
	DetoursUtils::DetachDetours(moduleName, {
		{ (PVOID *)(&ORIG_SV_ActivateServer), HOOKED_SV_ActivateServer },
		{ (PVOID *)(&ORIG_FinishRestore), HOOKED_FinishRestore },
		{ (PVOID *)(&ORIG_SetPaused), HOOKED_SetPaused },
		{ (PVOID *)(&ORIG__Host_RunFrame), HOOKED__Host_RunFrame },
		{ (PVOID *)(&ORIG__Host_RunFrame_Input), HOOKED__Host_RunFrame_Input },
		{ (PVOID *)(&ORIG__Host_RunFrame_Server), HOOKED__Host_RunFrame_Server },
		{ (PVOID *)(&ORIG_Host_AccumulateTime), HOOKED_Host_AccumulateTime },
		{ (PVOID *)(&ORIG_Cbuf_Execute), HOOKED_Cbuf_Execute },
		{ (PVOID *)(&ORIG_Cmd_ExecuteCommand), HOOKED_Cmd_ExecuteCommand }
	});

	Clear();
}

void EngineDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_SV_ActivateServer = nullptr;
	ORIG_FinishRestore = nullptr;
	ORIG_SetPaused = nullptr;
	ORIG__Host_RunFrame = nullptr;
	ORIG__Host_RunFrame_Input = nullptr;
	ORIG__Host_RunFrame_Server = nullptr;
	ORIG_Host_AccumulateTime = nullptr;
	ORIG_Cbuf_Execute = nullptr;
	ORIG_Cmd_ExecuteCommand = nullptr;
	pGameServer = nullptr;
	pM_bLoadgame = nullptr;
	shouldPreventNextUnpause = false;
	pIntervalPerTick = nullptr;
	pHost_Frametime = nullptr;
	pM_State = nullptr;
	pM_nSignonState = nullptr;
	pDemoplayer = nullptr;
}

float EngineDLL::GetTickrate() const
{
	if (pIntervalPerTick)
		return *pIntervalPerTick;

	return 0;
}

void EngineDLL::SetTickrate(float value)
{
	if (pIntervalPerTick)
		*pIntervalPerTick = value;
}

int EngineDLL::Demo_GetPlaybackTick() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall ***)(void*)>(demoplayer))[2](demoplayer);
}

int EngineDLL::Demo_GetTotalTicks() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall ***)(void*)>(demoplayer))[3](demoplayer);
}

bool EngineDLL::Demo_IsPlayingBack() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall ***)(void*)>(demoplayer))[5](demoplayer);
}

bool EngineDLL::Demo_IsPlaybackPaused() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall ***)(void*)>(demoplayer))[6](demoplayer);
}

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer_Func()
{
	bool result = ORIG_SV_ActivateServer();
	tas_pause.SetValue(0);

	EngineDevMsg("Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false"));

	if (ORIG_SetPaused && pM_bLoadgame && pGameServer)
	{
		if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
		{
			ORIG_SetPaused((void *)pGameServer, 0, true);
			EngineDevMsg("Pausing...\n");

			shouldPreventNextUnpause = true;
		}
	}

	if (_y_spt_afterframes_reset_on_server_activate.GetBool())
		clientDLL.ResetAfterframesQueue();
		

	return result;
}

void __fastcall EngineDLL::HOOKED_FinishRestore_Func(void* thisptr, int edx)
{
	EngineDevMsg("Engine call: FinishRestore();\n");

	if (ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
	{
		ORIG_SetPaused(thisptr, 0, true);
		EngineDevMsg("Pausing...\n");

		shouldPreventNextUnpause = true;
	}

	ORIG_FinishRestore(thisptr, edx);

	clientDLL.ResumeAfterframesQueue();
}

void __fastcall EngineDLL::HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused)
{
	if (pM_bLoadgame)
	{
		EngineDevMsg("Engine call: SetPaused( %s ); m_bLoadgame = %s\n", (paused ? "true" : "false"), (*pM_bLoadgame ? "true" : "false"));
	}
	else
	{
		EngineDevMsg("Engine call: SetPaused( %s );\n", (paused ? "true" : "false"));
	}

	if (paused == false)
	{
		if (shouldPreventNextUnpause)
		{
			EngineDevMsg("Unpause prevented.\n");
			shouldPreventNextUnpause = false;
			return;
		}
	}

	shouldPreventNextUnpause = false;
	return ORIG_SetPaused(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Func(float time)
{
	EngineDevMsg("_Host_RunFrame( %.8f ); m_nSignonState = %d;", time, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg("\n");

	ORIG__Host_RunFrame(time);

	EngineDevMsg("_Host_RunFrame end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick)
{
	EngineDevMsg("_Host_RunFrame_Input( %.8f, %d ); m_nSignonState = %d;", time, bFinalTick, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Input(accumulated_extra_samples, bFinalTick);

	EngineDevMsg("_Host_RunFrame_Input end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server_Func(int bFinalTick)
{
	EngineDevMsg("_Host_RunFrame_Server( %d ); m_nSignonState = %d;", bFinalTick, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Server(bFinalTick);

	EngineDevMsg("_Host_RunFrame_Server end.\n");
}

void __cdecl EngineDLL::HOOKED_Host_AccumulateTime_Func(float dt)
{
	if (tas_pause.GetBool())
	{
		*pHost_Realtime += dt;
		*pHost_Frametime = 0;
	}
	else
		ORIG_Host_AccumulateTime(dt);
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute_Func()
{
	EngineDevMsg("Cbuf_Execute(); m_nSignonState = %d;", *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG_Cbuf_Execute();

	EngineDevMsg("Cbuf_Execute() end.\n");
}

void * __cdecl EngineDLL::HOOKED_Cmd_ExecuteCommand_Func(CCommand & command, int src, int nClientSlot)
{
	scripts::g_Capture.SendCommand(command);
	return ORIG_Cmd_ExecuteCommand(command, src, nClientSlot);
}
