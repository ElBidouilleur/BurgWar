// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientElementLibrary.hpp>
#include <ClientLib/Components/LocalMatchComponent.hpp>
#include <NDK/World.hpp>
#include <NDK/Components/PhysicsComponent2D.hpp>
#include <NDK/Systems/PhysicsSystem2D.hpp>
#include <sol3/sol.hpp>

namespace bw
{
	void ClientElementLibrary::RegisterLibrary(sol::table& elementMetatable)
	{
		SharedElementLibrary::RegisterLibrary(elementMetatable);

		RegisterClientLibrary(elementMetatable);
	}

	void ClientElementLibrary::RegisterClientLibrary(sol::table& elementTable)
	{
		auto DealDamage = [this](const sol::table& entityTable, const Nz::Vector2f& origin, Nz::UInt16 /*damage*/, Nz::Rectf damageZone, float pushbackForce = 0.f)
		{
			// Client-side this function only applies push-back forces
			if (Nz::NumberEquals(pushbackForce, 0.f))
				return;

			const Ndk::EntityHandle& entity = AssertScriptEntity(entityTable);
			Ndk::World* world = entity->GetWorld();
			assert(world);

			world->GetSystem<Ndk::PhysicsSystem2D>().RegionQuery(damageZone, 0, 0xFFFFFFFF, 0xFFFFFFFF, [&](const Ndk::EntityHandle& hitEntity)
			{
				if (hitEntity->HasComponent<Ndk::PhysicsComponent2D>())
				{
					Ndk::PhysicsComponent2D& hitEntityPhys = hitEntity->GetComponent<Ndk::PhysicsComponent2D>();
					hitEntityPhys.AddImpulse(Nz::Vector2f::Normalize(hitEntityPhys.GetMassCenter(Nz::CoordSys_Global) - origin) * pushbackForce);
				}
			});
		};

		elementTable["DealDamage"] = sol::overload(DealDamage,
			[=](const sol::table& entityTable, const Nz::Vector2f& origin, Nz::UInt16 damage, Nz::Rectf damageZone) { DealDamage(entityTable, origin, damage, damageZone); });

		elementTable["GetLayerIndex"] = [](const sol::table& entityTable)
		{
			const Ndk::EntityHandle& entity = AssertScriptEntity(entityTable);

			return entity->GetComponent<LocalMatchComponent>().GetLayerIndex();
		};
	}
}
