#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "SPTLib\patterncontainer.hpp"
#include "tier0\basetypes.h"
#include "vguimatsurface\imatsystemsurface.h"
#include "vgui\ischeme.h"
#include "convar.h"

using std::uintptr_t;
using std::size_t;

typedef int(__cdecl *_StartDrawing) ();
typedef int(__cdecl *_FinishDrawing) ();

class VGui_MatSurfaceDLL : public IHookableNameFilter
{
public:
	VGui_MatSurfaceDLL() : IHookableNameFilter({ L"vguimatsurface.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	void NewTick();
	void DrawHUD(vrect_t* screen);
	void DrawTopRightHUD(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface);

	_StartDrawing ORIG_StartDrawing;
	_FinishDrawing ORIG_FinishDrawing;
protected:
	DetoursUtils::PatternContainer patternContainer;
	ConVar* cl_showpos;
	ConVar* cl_showfps;
	vgui::HFont font;

	Vector currentVel;
	Vector previousVel;
};