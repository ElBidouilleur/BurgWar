// Copyright (C) 2018 J�r�me Leclercq
// This file is part of the "Burgwar Shared" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Shared/MatchClientSession.hpp>
#include <Shared/Match.hpp>
#include <Shared/MapData.hpp>
#include <Shared/NetworkReactor.hpp>
#include <Shared/PlayerCommandStore.hpp>
#include <Shared/Terrain.hpp>
#include <iostream>

namespace bw
{
	void MatchClientSession::Disconnect()
	{
		m_bridge->Disconnect();
	}

	void MatchClientSession::HandleIncomingPacket(Nz::NetPacket&& packet)
	{
		m_commandStore.UnserializePacket(*this, std::move(packet));
	}

	void MatchClientSession::HandleIncomingPacket(const Packets::Auth& packet)
	{
		std::cout << "[Server] Auth request" << std::endl;

		SendPacket(Packets::AuthSuccess());

		const MapData& mapData = m_match.GetTerrain().GetMapData();

		Packets::MatchData matchData;
		matchData.backgroundColor = mapData.backgroundColor;
		matchData.tileSize = mapData.tileSize;

		matchData.layers.reserve(mapData.layers.size());
		for (auto& layer : mapData.layers)
		{
			auto& packetLayer = matchData.layers.emplace_back();
			packetLayer.height = static_cast<Nz::UInt16>(layer.height);
			packetLayer.width = static_cast<Nz::UInt16>(layer.width);
			packetLayer.tiles = layer.tiles;
		}

		SendPacket(matchData);
	}

	void MatchClientSession::HandleIncomingPacket(const Packets::HelloWorld& packet)
	{
		std::cout << "[Server] Hello world: " << packet.str << std::endl;

		Packets::HelloWorld hw;
		hw.str = "La belgique aurait d� gagner la coupe du monde 2018";

		SendPacket(hw);
	}
}
