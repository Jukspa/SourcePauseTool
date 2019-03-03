#pragma once
#include <map>
#include <string>
#include <vector>
#include "SPTLib\memutils.hpp"

#define PVOID void*

class PatternContainer
{
public:
	PatternContainer() {};
	int FindPatternIndex(PVOID* origPtr);
	void Init(const std::wstring& moduleName);
	void AddHook(PVOID functionToHook, PVOID* origPtr);
	void AddIndex(PVOID* origPtr, int index);
	void Hook();
	void Unhook();
private:
	std::map<int, int> patterns;
	std::vector<std::pair<PVOID *, PVOID>> entries;
	std::vector<PVOID*> functions;
	std::wstring moduleName;
};