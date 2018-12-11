#include "stdafx.h"
#include "framebulk_handler.hpp"
#include "..\..\utils\string_parsing.hpp"

namespace scripts
{
	typedef void(*CommandCallback) (FrameBulkInfo& frameBulkInfo);
	std::vector<CommandCallback> frameBulkHandlers;

	const std::string FIELD0_FILLED = "s**ljdbcgu";
	const std::string FIELD1_FILLED = "flrbud";
	const std::string FIELD2_FILLED = "jdu12r";
	const std::string EMPTY_FIELD = "-";
	const std::string NOOP_FIELD = ">";
	const char WILDCARD = '*';
	const char DELIMITER = '|';

	const auto STRAFE = std::pair<int, int>(0, 0);
	const auto STRAFE_TYPE = std::pair<int, int>(0, 1);
	const auto JUMP_TYPE = std::pair<int, int>(0, 2);
	const auto LGAGST = std::pair<int, int>(0, 3);
	const auto AUTOJUMP = std::pair<int, int>(0, 4);
	const auto DUCKSPAM = std::pair<int, int>(0, 5);
	const auto JUMPBUG = std::pair<int, int>(0, 6);
	const auto DUCK_BEFORE_COLLISION = std::pair<int, int>(0, 7);
	const auto DUCK_BEFORE_GROUND = std::pair<int, int>(0, 8);
	const auto USE_SPAM = std::pair<int, int>(0, 9);

	const auto FORWARD = std::pair<int, int>(1, 0);
	const auto LEFT = std::pair<int, int>(1, 1);
	const auto RIGHT = std::pair<int, int>(1, 2);
	const auto BACK = std::pair<int, int>(1, 3);
	const auto UP = std::pair<int, int>(1, 4);
	const auto DOWN = std::pair<int, int>(1, 5);

	const auto JUMP = std::pair<int, int>(2, 0);
	const auto DUCK = std::pair<int, int>(2, 1);
	const auto USE = std::pair<int, int>(2, 2);
	const auto ATTACK1 = std::pair<int, int>(2, 3);
	const auto ATTACK2 = std::pair<int, int>(2, 4);
	const auto RELOAD = std::pair<int, int>(2, 5);

	const auto YAW_KEY = std::pair<int, int>(3, 0);
	const auto PITCH_KEY = std::pair<int, int>(4, 0);
	const auto TICKS = std::pair<int, int>(5, 0);
	const auto COMMANDS = std::pair<int, int>(6, 0);

	void Field1(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[STRAFE] == "s")
		{
			frameBulkInfo.AddCommand("tas_strafe 1", STRAFE);

			if (!frameBulkInfo.IsInt(JUMP_TYPE) || !frameBulkInfo.IsInt(STRAFE_TYPE))
				throw std::exception("Jump type or strafe type was not an integer");

			frameBulkInfo.AddCommand("tas_strafe_jumptype " + frameBulkInfo[JUMP_TYPE], JUMP_TYPE);
			frameBulkInfo.AddCommand("tas_strafe_type " + frameBulkInfo[STRAFE_TYPE], STRAFE_TYPE);
		}
		else
			frameBulkInfo.AddCommand("tas_strafe 0", STRAFE);

		if (frameBulkInfo[AUTOJUMP] == "j")
			frameBulkInfo.AddCommand("y_spt_autojump 1", AUTOJUMP);
		else
			frameBulkInfo.AddCommand("y_spt_autojump 0", AUTOJUMP);

		frameBulkInfo.AddPlusMinusCmd("y_spt_duckspam", frameBulkInfo[DUCKSPAM] == "d", DUCKSPAM);

		// todo
#pragma warning(push)
#pragma warning(disable:4390)
		if (frameBulkInfo[USE_SPAM] == "u");
		if (frameBulkInfo[LGAGST] == "l");
		if (frameBulkInfo[JUMPBUG] == "b");
		if (frameBulkInfo[DUCK_BEFORE_COLLISION] == "c");
		if (frameBulkInfo[DUCK_BEFORE_GROUND] == "g");
#pragma warning(pop)
	}

	void Field2(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("forward", frameBulkInfo[FORWARD] == "f", FORWARD);
		frameBulkInfo.AddPlusMinusCmd("moveleft", frameBulkInfo[LEFT] == "l", LEFT);
		frameBulkInfo.AddPlusMinusCmd("moveright", frameBulkInfo[RIGHT] == "r", RIGHT);
		frameBulkInfo.AddPlusMinusCmd("back", frameBulkInfo[BACK] == "b", BACK);
		frameBulkInfo.AddPlusMinusCmd("moveup", frameBulkInfo[UP] == "u", UP);
		frameBulkInfo.AddPlusMinusCmd("movedown", frameBulkInfo[DOWN] == "d", DOWN);
	}

	void Field3(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("jump", frameBulkInfo[JUMP] == "j" || frameBulkInfo[AUTOJUMP] == "j", JUMP);
		frameBulkInfo.AddPlusMinusCmd("duck", frameBulkInfo[DUCK] == "d", DUCK);
		frameBulkInfo.AddPlusMinusCmd("use", frameBulkInfo[USE] == "u", USE);
		frameBulkInfo.AddPlusMinusCmd("attack", frameBulkInfo[ATTACK1] == "1", ATTACK1);
		frameBulkInfo.AddPlusMinusCmd("attack2", frameBulkInfo[ATTACK2] == "2", ATTACK2);
		frameBulkInfo.AddPlusMinusCmd("reload", frameBulkInfo[RELOAD] == "r", RELOAD);
	}

	void Field4_5(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo.IsFloat(YAW_KEY))
		{
			if (frameBulkInfo[STRAFE] == "s")
				frameBulkInfo.AddCommand("tas_strafe_yaw " + frameBulkInfo[YAW_KEY], YAW_KEY);
			else
				frameBulkInfo.AddCommand("_y_spt_setyaw " + frameBulkInfo[YAW_KEY], YAW_KEY);
		}
		else if (frameBulkInfo[YAW_KEY] != EMPTY_FIELD)
			throw std::exception("Unable to parse the yaw angle");

		if (frameBulkInfo.IsFloat(PITCH_KEY))
			frameBulkInfo.AddCommand("_y_spt_setpitch " + frameBulkInfo[PITCH_KEY], PITCH_KEY);
		else if (frameBulkInfo[PITCH_KEY] != EMPTY_FIELD)
			throw std::exception("Unable to parse the pitch angle");
	}

	void Field6(FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo.IsInt(TICKS))
			throw std::exception("Tick value was not an integer");

		int ticks = std::atoi(frameBulkInfo[TICKS].c_str());
		frameBulkInfo.data.ticks = ticks;
	}

	void Field7(FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo[COMMANDS].empty())
			frameBulkInfo.data.AddRepeatingCommand(frameBulkInfo[COMMANDS]);
	}

	void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD0_FILLED, 0);
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD1_FILLED, 1);
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD2_FILLED, 2);
	}

	void InitHandlers()
	{
		frameBulkHandlers.push_back(ValidateFieldFlags);
		frameBulkHandlers.push_back(Field1);
		frameBulkHandlers.push_back(Field2);
		frameBulkHandlers.push_back(Field3);
		frameBulkHandlers.push_back(Field4_5);
		frameBulkHandlers.push_back(Field6);
		frameBulkHandlers.push_back(Field7);
	}

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkHandlers.empty())
			InitHandlers();

		for (auto handler : frameBulkHandlers)
			handler(frameBulkInfo);

		return frameBulkInfo.data;
	}

	void FrameBulkOutput::AddCommand(const std::string& newCmd)
	{
		initialCommand.push_back(';');
		initialCommand += newCmd;
	}

	void FrameBulkOutput::AddCommand(char initChar, const std::string& newCmd)
	{
		initialCommand.push_back(';');
		initialCommand.push_back(initChar);
		initialCommand += newCmd;
	}

	void FrameBulkOutput::AddRepeatingCommand(const std::string & newCmd)
	{
		repeatingCommand.push_back(';');
		repeatingCommand += newCmd;
	}

	FrameBulkInfo::FrameBulkInfo(std::istringstream& stream)
	{
		int section = 0;
		std::string line;

		do
		{
			std::getline(stream, line, DELIMITER);

			// The sections after the first three are single string (could map section index to value type here but probably not worth the effort)
			if (section > 2)
			{
				dataMap[std::make_pair(section, 0)] = line;
			}
			else
			{
				for (size_t i = 0; i < line.size(); ++i)
					dataMap[std::make_pair(section, i)] = line[i];
			}

			++section;
		} while (stream.good());
	}

	const std::string& FrameBulkInfo::operator[](std::pair<int, int> i)
	{
		if (dataMap.find(i) == dataMap.end())
		{
			char buffer[50];
			std::snprintf(buffer, 50, "Unable to find index (%i, %i) in frame bulk", i.first, i.second);

			throw std::exception(buffer);
		}

		return dataMap[i];
	}

	bool FrameBulkInfo::IsInt(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<int>(value);
	}

	bool FrameBulkInfo::IsFloat(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<float>(value);
	}

	void FrameBulkInfo::AddCommand(const std::string & cmd, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		data.AddCommand(cmd);
	}

	void FrameBulkInfo::AddPlusMinusCmd(const std::string& command, bool set, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		if (set)
			data.AddCommand('+', command);
		else
			data.AddCommand('-', command);
	}

	bool FrameBulkInfo::NoopField(const std::pair<int, int>& field)
	{
		return this->operator[](field) == NOOP_FIELD;
	}

	void FrameBulkInfo::ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index)
	{
		for (size_t i = 0; i < fields.length(); ++i)
		{
			if (fields[i] != WILDCARD)
			{
				auto& value = frameBulkInfo[std::make_pair(index, i)];
				if (value[0] != fields[i] && value[0] != '-')
				{
					std::ostringstream os;
					os << "Expected " << fields[i] << ", got " << value << " in field: " << fields;
					throw std::exception(os.str().c_str());
				}
			}
		}
	}
}
