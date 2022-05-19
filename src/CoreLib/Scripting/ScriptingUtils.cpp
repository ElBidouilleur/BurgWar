// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Scripting/ScriptingUtils.hpp>
#include <CoreLib/Components/ScriptComponent.hpp>

namespace bw
{
	std::shared_ptr<ScriptedElement> AssertScriptElement(const sol::table& entityTable)
	{
		std::shared_ptr<ScriptedElement> element = RetrieveScriptElement(entityTable);
		if (!element)
			TriggerLuaError(entityTable.lua_state(), "invalid element");

		return element;
	}

	entt::handle AssertScriptEntity(const sol::table& entityTable)
	{
		entt::handle entity = RetrieveScriptEntity(entityTable);
		if (entity == entt::null)
			TriggerLuaError(entityTable.lua_state(), "invalid entity");

		return entity;
	}

	std::shared_ptr<ScriptedElement> RetrieveScriptElement(const sol::table& entityTable)
	{
		sol::object entityObject = entityTable["_Element"];
		if (!entityObject)
			return nullptr;

		return entityObject.as<std::shared_ptr<ScriptedElement>>();
	}

	entt::handle RetrieveScriptEntity(const sol::table& entityTable)
	{
		sol::object entityObject = entityTable["_Entity"];
		if (!entityObject)
			return entt::null;

		return entityObject.as<entt::handle>();
	}

	std::optional<sol::object> TranslateEntityToLua(entt::handle entity)
	{
		if (!entity)
			return std::nullopt;

		if (ScriptComponent* scriptComponent = entity.try_get<ScriptComponent>())
			return scriptComponent->GetTable();
		else
			return std::nullopt;
	}

	[[noreturn]] void TriggerLuaError(lua_State* L, const std::string& errMessage)
	{
		luaL_error(L, errMessage.c_str());
		std::abort();
	}

	[[noreturn]] void TriggerLuaArgError(lua_State* L, int argIndex, const char* errMessage)
	{
		luaL_argerror(L, argIndex, errMessage);
		std::abort();
	}

	[[noreturn]] void TriggerLuaArgError(lua_State* L, int argIndex, const std::string& errMessage)
	{
		luaL_argerror(L, argIndex, errMessage.c_str());
		std::abort();
	}
}
