// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Scripting/SharedEntityLibrary.hpp>
#include <CoreLib/CustomInputController.hpp>
#include <CoreLib/PlayerMovementController.hpp>
#include <CoreLib/Colliders.hpp>
#include <CoreLib/Utils.hpp>
#include <CoreLib/Components/CollisionDataComponent.hpp>
#include <CoreLib/Components/HealthComponent.hpp>
#include <CoreLib/Components/InputComponent.hpp>
#include <CoreLib/Components/PlayerMovementComponent.hpp>
#include <CoreLib/Components/ScriptComponent.hpp>
#include <CoreLib/Components/WeaponWielderComponent.hpp>
#include <CoreLib/Scripting/ScriptingUtils.hpp> 
#include <CoreLib/Scripting/SharedScriptingLibrary.hpp>
#include <Nazara/Utils/CallOnExit.hpp>
#include <Nazara/Physics2D/Components/RigidBody2DComponent.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <sol/sol.hpp>

namespace bw
{
	namespace
	{
		Collider ParseCollider(const sol::table& colliderData)
		{
			ColliderPhysics physics;

			sol::table colliderTable;
			sol::object metatableOpt = colliderData[sol::metatable_key];
			if (!metatableOpt)
			{
				colliderTable = colliderData["Collider"];

				metatableOpt = colliderTable[sol::metatable_key];
				if (!metatableOpt)
					throw std::runtime_error("invalid collider");

				physics.elasticity = colliderData.get_or("Elasticity", physics.elasticity);
				physics.friction = colliderData.get_or("Friction", physics.friction);
				physics.isTrigger = colliderData.get_or("IsTrigger", physics.isTrigger);
				physics.surfaceVelocity = colliderData.get_or("SurfaceVelocity", physics.surfaceVelocity);
				physics.colliderId = colliderData.get_or("ColliderType", physics.colliderId);
			}
			else
				colliderTable = colliderData;

			sol::table metatable = metatableOpt.as<sol::table>();

			std::string typeName = metatable["__name"];

			if (typeName == "rect")
			{
				RectangleCollider collider;
				collider.physics = physics;

				collider.data = colliderTable.as<Nz::Rectf>();

				return collider;
			}
			else if (typeName == "circle")
			{
				CircleCollider collider;
				collider.physics = physics;

				collider.offset = colliderTable["origin"];
				collider.radius = colliderTable["radius"];

				return collider;
			}
			else if (typeName == "segment")
			{
				SegmentCollider collider;
				collider.physics = physics;

				collider.from = colliderTable["from"];
				collider.to = colliderTable["to"];

				collider.fromNeighbor = collider.from;
				collider.toNeighbor = collider.to;

				if (colliderData != colliderTable)
				{
					collider.fromNeighbor = colliderData.get_or("FromNeighbor", collider.fromNeighbor);
					collider.toNeighbor = colliderData.get_or("ToNeighbor", collider.toNeighbor);
				}

				return collider;
			}
			else
				throw std::runtime_error("invalid collider type " + typeName);
		};

		sol::table ColliderToTable(sol::state_view& state, const std::vector<Collider>& colliders)
		{
			sol::table colliderTable = state.create_table(int(colliders.size()), 0);

			int index = 1;
			for (const Collider& colliderInfo : colliders)
			{
				sol::table colliderData = state.create_table(0, 6);
				std::visit([&](auto&& collider)
				{
					using T = std::decay_t<decltype(collider)>;

					colliderData["Elasticity"] = collider.physics.elasticity;
					colliderData["Friction"] = collider.physics.friction;
					colliderData["IsTrigger"] = collider.physics.isTrigger;
					colliderData["SurfaceVelocity"] = collider.physics.surfaceVelocity;
					colliderData["ColliderType"] = collider.physics.colliderId;

					if constexpr (std::is_same_v<T, CircleCollider>)
					{
						lua_createtable(state, 0, 2);
						luaL_setmetatable(state, "circle");
						sol::stack_table circle(state);
						circle["origin"] = collider.offset;
						circle["radius"] = collider.radius;

						colliderData["Collider"] = circle;
					}
					else if constexpr (std::is_same_v<T, RectangleCollider>)
					{
						colliderData["Collider"] = collider.data;
					}
					else if constexpr (std::is_same_v<T, SegmentCollider>)
					{
						lua_createtable(state, 0, 2);
						luaL_setmetatable(state, "segment");
						sol::stack_table segment(state);
						segment["from"] = collider.from;
						segment["to"] = collider.to;

						colliderData["Collider"] = segment;

						if (collider.fromNeighbor != collider.from)
							colliderData["FromNeighbor"] = collider.fromNeighbor;

						if (collider.toNeighbor != collider.to)
							colliderData["ToNeighbor"] = collider.toNeighbor;
					}
					else
						static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");

				}, colliderInfo);

				colliderTable[index++] = colliderData;
			}

			return colliderTable;
		}
	}

	void SharedEntityLibrary::RegisterLibrary(sol::table& elementMetatable)
	{
		RegisterSharedLibrary(elementMetatable);
	}

	void SharedEntityLibrary::InitRigidBody(lua_State* /*L*/, entt::handle entity, float mass)
	{
		auto& entityNode = entity.get<Nz::NodeComponent>();
		auto& entityPhys = entity.emplace<Nz::RigidBody2DComponent>();
		entityPhys.SetMass(mass);

		// Temp fix because Nazara
		entityPhys.SetRotation(AngleFromQuaternion(entityNode.GetRotation(Nz::CoordSys::Global)));
	}

	void SharedEntityLibrary::SetDirection(lua_State* L, entt::handle entity, const Nz::Vector2f& upVector)
	{
		Nz::RadianAnglef angle(std::atan2(upVector.y, upVector.x) + Nz::Pi<float> / 2.f);

		if (Nz::RigidBody2DComponent* rigidBody = entity.try_get<Nz::RigidBody2DComponent>())
			rigidBody->SetRotation(angle);

		auto& nodeComponent = entity.get<Nz::NodeComponent>();
		nodeComponent.SetRotation(angle);
	}

	void SharedEntityLibrary::SetMass(lua_State* /*L*/, entt::handle entity, float mass, bool recomputeMomentOfInertia)
	{
		Nz::RigidBody2DComponent* rigidBody = entity.try_get<Nz::RigidBody2DComponent>();
		if (!rigidBody)
			return;

		rigidBody->SetMass(mass, recomputeMomentOfInertia);
	}

	void SharedEntityLibrary::SetMomentOfInertia(lua_State* /*L*/, entt::handle entity, float momentOfInertia)
	{
		Nz::RigidBody2DComponent* rigidBody = entity.try_get<Nz::RigidBody2DComponent>();
		if (!rigidBody)
			return;

		rigidBody->SetMomentOfInertia(momentOfInertia);
	}

	void SharedEntityLibrary::SetPosition(lua_State* L, entt::handle entity, const Nz::Vector2f& position)
	{
		if (Nz::RigidBody2DComponent* rigidBody = entity.try_get<Nz::RigidBody2DComponent>())
			rigidBody->SetPosition(position);

		auto& nodeComponent = entity.get<Nz::NodeComponent>();
		nodeComponent.SetPosition(position);
	}

	void SharedEntityLibrary::SetRotation(lua_State* L, entt::handle entity, const Nz::DegreeAnglef& rotation)
	{
		if (Nz::RigidBody2DComponent* rigidBody = entity.try_get<Nz::RigidBody2DComponent>())
			rigidBody->SetRotation(rotation);

		auto& nodeComponent = entity.get<Nz::NodeComponent>();
		nodeComponent.SetRotation(rotation);
	}

	void SharedEntityLibrary::UpdatePlayerJumpHeight(lua_State* L, entt::handle entity, float jumpHeight, float jumpHeightBoost)
	{
		PlayerMovementComponent* playerMovementComponent = entity.try_get<PlayerMovementComponent>();
		if (!playerMovementComponent)
			TriggerLuaArgError(L, 1, "entity has no player movement");

		playerMovementComponent->UpdateJumpHeight(jumpHeight);
		playerMovementComponent->UpdateJumpBoostHeight(jumpHeightBoost);
	}

	void SharedEntityLibrary::UpdatePlayerMovement(lua_State* L, entt::handle entity, float movementSpeed)
	{
		PlayerMovementComponent* playerMovementComponent = entity.try_get<PlayerMovementComponent>();
		if (!playerMovementComponent)
			TriggerLuaArgError(L, 1, "entity has no player movement");

		playerMovementComponent->UpdateMovementSpeed(movementSpeed);
	}

	void SharedEntityLibrary::RegisterSharedLibrary(sol::table& elementMetatable)
	{
		elementMetatable["AddForce"] = LuaFunction([this](const sol::table& entityTable, const Nz::Vector2f& force)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				Ndk::PhysicsComponent2D& hitEntityPhys = entity.get<Ndk::PhysicsComponent2D>();
				hitEntityPhys.AddForce(force);
			}
		});

		elementMetatable["ApplyImpulse"] = LuaFunction([this](const sol::table& entityTable, const Nz::Vector2f& force)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				Ndk::PhysicsComponent2D& hitEntityPhys = entity.get<Ndk::PhysicsComponent2D>();
				hitEntityPhys.AddImpulse(force);
			}
		});

		elementMetatable["Damage"] = LuaFunction([](const sol::table& entityTable, Nz::UInt16 damage, std::optional<sol::table> attackerEntity)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<HealthComponent>())
				return;

			auto& entityHealth = entity.get<HealthComponent>();
			entityHealth.Damage(damage, (attackerEntity) ? RetrieveScriptEntity(*attackerEntity) : entt::null);
		});

		elementMetatable["ForceSleep"] = LuaFunction([](const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
				physComponent.ForceSleep();
			}
		});
		
		elementMetatable["GetColliders"] = LuaFunction([](sol::this_state L, const sol::table& entityTable) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<CollisionDataComponent>())
				return sol::nil;

			sol::state_view state(L);

			return ColliderToTable(state, entity.get<CollisionDataComponent>().GetColliders());
		});
		
		elementMetatable["GetInputController"] = LuaFunction([](sol::this_state L, const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (!entity->HasComponent<InputComponent>())
				TriggerLuaArgError(L, 1, "entity has no inputs");

			return entity.get<InputComponent>().GetController();
		});

		elementMetatable["GetHealth"] = LuaFunction([](const sol::table& entityTable) -> Nz::UInt16
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<HealthComponent>())
				return 0;

			auto& entityHealth = entity.get<HealthComponent>();
			return entityHealth.GetHealth();
		});

		elementMetatable["GetMass"] = LuaFunction([](sol::this_state L, const sol::table& entityTable) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
				return sol::make_object(L, physComponent.GetMass());
			}
			else
				return sol::nil;
		});

		elementMetatable["GetMomentOfInertia"] = LuaFunction([](sol::this_state L, const sol::table& entityTable) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
				return sol::make_object(L, physComponent.GetMomentOfInertia());
			}
			else
				return sol::nil;
		});

		elementMetatable["GetPlayerMovementController"] = LuaFunction([](sol::this_state L, const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (!entity->HasComponent<PlayerMovementComponent>())
				TriggerLuaArgError(L, 1, "entity has no player movement");

			return entity.get<PlayerMovementComponent>().GetController();
		});

		elementMetatable["GetPlayerMovementSpeed"] = LuaFunction([](sol::this_state L, const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (!entity->HasComponent<PlayerMovementComponent>())
				TriggerLuaArgError(L, 1, "entity has no player movement");

			return entity.get<PlayerMovementComponent>().GetMovementSpeed();
		});

		elementMetatable["GetPlayerJumpHeight"] = LuaFunction([](sol::this_state L, const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (!entity->HasComponent<PlayerMovementComponent>())
				TriggerLuaArgError(L, 1, "entity has no player movement");

			auto& movementComponent = entity.get<PlayerMovementComponent>();
			float jumpHeight = movementComponent.GetJumpHeight();
			float jumpBoostHeigh = movementComponent.GetJumpBoostHeight();

			return std::make_pair(jumpHeight, jumpBoostHeigh);
		});

		elementMetatable["GetUpVector"] = LuaFunction([](const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			auto& nodeComponent = entity.get<Nz::NodeComponent>();
			return Nz::Vector2f(nodeComponent.GetUp());
		});

		elementMetatable["GetVelocity"] = LuaFunction([](const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<Ndk::PhysicsComponent2D>())
				return Nz::Vector2f::Zero();

			auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
			return physComponent.GetVelocity();
		});

		elementMetatable["Heal"] = LuaFunction([](const sol::table& entityTable, Nz::UInt16 value, std::optional<sol::table> healerEntity)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<HealthComponent>())
				return;

			auto& entityHealth = entity.get<HealthComponent>();
			entityHealth.Heal(value, (healerEntity) ? RetrieveScriptEntity(*healerEntity) : entt::null);
		});

		elementMetatable["InitWeaponWielder"] = LuaFunction([](const sol::table& entityTable, const sol::table& wielderData)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& wielderComponent = entity.emplace<WeaponWielderComponent>();
			wielderComponent.SetWeaponOffset(wielderData["WeaponOffset"]);
		});

		elementMetatable["IsFullHealth"] = LuaFunction([](const sol::table& entityTable) -> bool
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<HealthComponent>())
				return false;

			auto& entityHealth = entity.get<HealthComponent>();
			return entityHealth.GetHealth() >= entityHealth.GetMaxHealth();
		});

		elementMetatable["InitRigidBody"] = LuaFunction([this](sol::this_state L, const sol::table& entityTable, float mass)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			this->InitRigidBody(L, entity, mass);
		});

		elementMetatable["IsPlayerOnGround"] = LuaFunction([](sol::this_state L, const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (!entity->HasComponent<PlayerMovementComponent>())
				TriggerLuaArgError(L, 1, "entity has no player movement");

			return entity.get<PlayerMovementComponent>().IsOnGround();
		});

		elementMetatable["IsSleeping"] = LuaFunction([](const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
				return entity.get<Ndk::PhysicsComponent2D>().IsSleeping();
			else
				return false;
		});

		elementMetatable["OverrideMovementController"] = LuaFunction([this](const sol::table& entityTable, sol::main_protected_function fn)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (entity->HasComponent<Ndk::PhysicsComponent2D>())
			{
				Ndk::PhysicsComponent2D& hitEntityPhys = entity.get<Ndk::PhysicsComponent2D>();
				if (fn)
				{
					hitEntityPhys.SetVelocityFunction([entity, fn = std::move(fn)](Nz::RigidBody2D& body2D, const Nz::Vector2f& gravity, float damping, float deltaTime)
					{
						auto& entityScript = entity.get<ScriptComponent>();

						Nz::Vector2f overridedGravity = gravity;
						float overridedDamping = damping;

						auto result = fn(gravity, damping, deltaTime);
						if (result)
							sol::tie(overridedGravity, overridedDamping) = result;
						else
						{
							sol::error err = result;
							bwLog(entityScript.GetLogger(), LogLevel::Error, "Movement controller callback failed: {0}", err.what());
						}

						body2D.UpdateVelocity(overridedGravity, overridedDamping, deltaTime);
					});
				}
				else
					hitEntityPhys.SetVelocityFunction(nullptr);
			}
		});

		elementMetatable["Remove"] = LuaFunction([](const sol::table& entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			entity->Kill();
		});

		elementMetatable["SetAngularVelocity"] = LuaFunction([](const sol::table& entityTable, const Nz::DegreeAnglef& velocity)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<Ndk::PhysicsComponent2D>())
				return;

			auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
			physComponent.SetAngularVelocity(velocity);
		});

		elementMetatable["SetColliders"] = LuaFunction([](sol::this_state L, const sol::table& entityTable, const sol::table& colliderTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			std::size_t colliderCount = colliderTable.size();

			auto& entityNode = entity.get<Nz::NodeComponent>();
			auto& entityCollData = entity.emplace<CollisionDataComponent>();

			if (colliderCount <= 1)
			{
				// Only one collider passed in a table or directly

				try
				{
					if (colliderCount == 0)
						entityCollData.AddCollider(ParseCollider(colliderTable));
					else
						entityCollData.AddCollider(ParseCollider(colliderTable[1]));
				}
				catch (const std::exception& e)
				{
					TriggerLuaArgError(L, 2, "invalid collider: " + std::string(e.what()));
				}
			}
			else
			{
				// Multiple colliders passed in a table
				for (std::size_t i = 0; i < colliderCount; ++i)
				{
					try
					{
						entityCollData.AddCollider(ParseCollider(colliderTable[i + 1]));
					}
					catch (const std::exception& e)
					{
						TriggerLuaArgError(L, 2, "invalid collider #" + std::to_string(i + 1) + ": " + std::string(e.what()));
					}
				}
			}

			entity.emplace<Ndk::CollisionComponent2D>(entityCollData.BuildCollider(entityNode.GetScale().y));
		});

		elementMetatable["SetDirection"] = LuaFunction([this](sol::this_state L, const sol::table& entityTable, const Nz::Vector2f& upVector)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity)
				return;

			SetDirection(L, entity, upVector);
		});

		auto SetMass = [this](sol::this_state L, const sol::table& entityTable, float mass, bool recomputeMomentOfInertia = false)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			this->SetMass(L, entity, mass, recomputeMomentOfInertia);
		};

		elementMetatable["SetMass"] = sol::overload(
			LuaFunction([=](sol::this_state L, const sol::table& entityTable, float mass)
			{
				SetMass(L, entityTable, mass); 
			}),
			LuaFunction(SetMass)
		);

		elementMetatable["SetMomentOfInertia"] = LuaFunction([this](sol::this_state L, const sol::table& entityTable, float momentum)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (momentum < 0.f)
				TriggerLuaArgError(L, 2, "moment of inertia must be positive");

			SetMomentOfInertia(L, entity, momentum);
		});

		elementMetatable["SetPosition"] = LuaFunction([this](sol::this_state L, const sol::table& entityTable, const Nz::Vector2f& position)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity)
				return;

			SetPosition(L, entity, position);
		});

		elementMetatable["SetRotation"] = LuaFunction([this](sol::this_state L, const sol::table& entityTable, const Nz::DegreeAnglef& rotation)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity)
				return;

			SetRotation(L, entity, rotation);
		});

		elementMetatable["SetVelocity"] = LuaFunction([](const sol::table& entityTable, const Nz::Vector2f& velocity)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity || !entity->HasComponent<Ndk::PhysicsComponent2D>())
				return;

			auto& physComponent = entity.get<Ndk::PhysicsComponent2D>();
			physComponent.SetVelocity(velocity);
		});

		elementMetatable["UpdateInputs"] = LuaFunction([](const sol::table& entityTable, const PlayerInputData& inputs)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity || !entity->HasComponent<InputComponent>())
				return;

			auto& entityInputs = entity.get<InputComponent>();
			entityInputs.UpdateInputs(inputs);
		});
		
		elementMetatable["UpdateInputController"] = LuaFunction([this](const sol::table& entityTable, std::shared_ptr<InputController> controller)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity || !entity->HasComponent<InputComponent>())
				return;

			auto& entityInputs = entity.get<InputComponent>();
			entityInputs.UpdateController(std::move(controller));
		});

		elementMetatable["UpdatePlayerMovementController"] = LuaFunction([](sol::this_state L, const sol::table& entityTable, sol::optional<std::shared_ptr<PlayerMovementController>> controller)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			if (!entity->HasComponent<PlayerMovementComponent>())
				TriggerLuaArgError(L, 1, "entity has no player movement");

			if (controller)
				return entity.get<PlayerMovementComponent>().UpdateController(std::move(*controller));
			else
				return entity.get<PlayerMovementComponent>().UpdateController(nullptr);
		});

		elementMetatable["UpdatePlayerMovementSpeed"] = LuaFunction([&](sol::this_state L, const sol::table& entityTable, float newSpeed)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			return UpdatePlayerMovement(L, entity, newSpeed);
		});

		elementMetatable["UpdatePlayerJumpHeight"] = LuaFunction([&](sol::this_state L, const sol::table& entityTable, float newJumpHeight, float newJumpBoostHeight)
		{
			entt::handle entity = AssertScriptEntity(entityTable);
			return UpdatePlayerJumpHeight(L, entity, newJumpHeight, newJumpBoostHeight);
		});
	}
}
