// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CORELIB_MATCH_HPP
#define BURGWAR_CORELIB_MATCH_HPP

#include <Nazara/Core/ByteArray.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <Nazara/Network/UdpSocket.hpp>
#include <CoreLib/AssetStore.hpp>
#include <CoreLib/LogSystem/MatchLogger.hpp>
#include <CoreLib/Map.hpp>
#include <CoreLib/MatchSessions.hpp>
#include <CoreLib/SharedMatch.hpp>
#include <CoreLib/TerrainLayer.hpp>
#include <CoreLib/Protocol/NetworkStringStore.hpp>
#include <CoreLib/Scripting/ScriptingContext.hpp>
#include <CoreLib/Scripting/ServerEntityStore.hpp>
#include <CoreLib/Scripting/ServerWeaponStore.hpp>
#include <tsl/hopscotch_map.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bw
{
	class BurgApp;
	class Player;
	class ServerGamemode;
	class ServerScriptingLibrary;
	class Terrain;

	using PlayerHandle = Nz::ObjectHandle<Player>;

	class Match : public SharedMatch
	{
		friend class MatchClientSession;

		public:
			struct Asset;
			struct ClientScript;

			Match(BurgApp& app, std::string matchName, std::filesystem::path gamemodeFolder, Map map, std::size_t maxPlayerCount, float tickDuration);
			Match(const Match&) = delete;
			Match(Match&&) = delete;
			~Match();

			template<typename T> void BuildClientAssetListPacket(T& clientAsset) const;
			template<typename T> void BuildClientScriptListPacket(T& clientScript) const;

			void ForEachEntity(std::function<void(const Ndk::EntityHandle& entity)> func) override;
			template<typename F> void ForEachPlayer(F&& func);

			inline BurgApp& GetApp();
			inline AssetStore& GetAssetStore();
			bool GetClientScript(const std::string& filePath, const ClientScript** clientScriptData);
			ServerEntityStore& GetEntityStore() override;
			const ServerEntityStore& GetEntityStore() const override;
			inline const std::shared_ptr<ServerGamemode>& GetGamemode();
			inline const std::filesystem::path& GetGamemodePath() const;
			TerrainLayer& GetLayer(LayerIndex layerIndex) override;
			const TerrainLayer& GetLayer(LayerIndex layerIndex) const override;
			LayerIndex GetLayerCount() const override;
			inline sol::state& GetLuaState();
			inline const Packets::MatchData& GetMatchData() const;
			inline const NetworkStringStore& GetNetworkStringStore() const;
			inline MatchSessions& GetSessions();
			inline const MatchSessions& GetSessions() const;
			inline const std::shared_ptr<ServerScriptingLibrary>& GetScriptingLibrary() const;
			inline Terrain& GetTerrain();
			inline const Terrain& GetTerrain() const;
			ServerWeaponStore& GetWeaponStore() override;
			const ServerWeaponStore& GetWeaponStore() const override;

			void InitDebugGhosts();

			void Leave(Player* player);
			bool Join(Player* player);

			void RegisterAsset(const std::filesystem::path& assetPath);
			void RegisterAsset(std::string assetPath, Nz::UInt64 assetSize, Nz::ByteArray assetChecksum);
			void RegisterClientScript(const std::filesystem::path& clientScript);

			void ReloadAssets();
			void ReloadScripts();

			void Update(float elapsedTime);

			Match& operator=(const Match&) = delete;
			Match& operator=(Match&&) = delete;

			struct Asset
			{
				Nz::ByteArray checksum;
				Nz::UInt64 size;
				std::string path;
			};

			struct ClientScript
			{
				Nz::ByteArray checksum;
				std::vector<Nz::UInt8> content;
			};

		private:
			void BuildMatchData();
			void OnPlayerReady(Player* player);
			void OnTick(bool lastTick) override;

			struct Debug
			{
				Nz::UdpSocket socket;
				Nz::UInt64 lastBroadcastTime = 0;
			};

			std::filesystem::path m_gamemodePath;
			std::optional<AssetStore> m_assetStore;
			std::optional<Debug> m_debug;
			std::optional<ServerEntityStore> m_entityStore;
			std::optional<ServerWeaponStore> m_weaponStore;
			std::size_t m_maxPlayerCount;
			std::shared_ptr<ServerGamemode> m_gamemode;
			std::shared_ptr<ScriptingContext> m_scriptingContext;
			std::shared_ptr<ServerScriptingLibrary> m_scriptingLibrary;
			std::string m_name;
			std::unique_ptr<Terrain> m_terrain;
			std::vector<PlayerHandle> m_players;
			mutable Packets::MatchData m_matchData;
			tsl::hopscotch_map<std::string, Asset> m_assets;
			tsl::hopscotch_map<std::string, ClientScript> m_clientScripts;
			BurgApp& m_app;
			Map m_map;
			MatchSessions m_sessions;
			NetworkStringStore m_networkStringStore;
	};
}

#include <CoreLib/Match.inl>

#endif
