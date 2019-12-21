// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Server/ServerApp.hpp>
#include <CoreLib/NetworkSessionManager.hpp>
#include <Nazara/Core/Thread.hpp>

namespace bw
{
	ServerApp::ServerApp(int argc, char* argv[]) :
	Application(argc, argv),
	BurgApp(LogSide::Server)
	{
		RegisterServerConfig();

		if (!m_config.LoadFromFile("serverconfig.lua"))
			throw std::runtime_error("Failed to load config file");

		Map map = Map::LoadFromBinary(GetConfig().GetStringOption("GameSettings.MapFile"));
		float tickRate = GetConfig().GetFloatOption<float>("GameSettings.TickRate");

		m_match = std::make_unique<Match>(*this, "local", "gamemodes/test", std::move(map), 64, 1.f / tickRate);
		m_match->GetSessions().CreateSessionManager<NetworkSessionManager>(Nz::UInt16(14768), 64);
	}

	int ServerApp::Run()
	{
		while (Application::Run())
		{
			BurgApp::Update();

			m_match->Update(GetUpdateTime());

			//TODO: Sleep only when server is not overloaded
			Nz::Thread::Sleep(1);
		}

		return 0;
	}

	void ServerApp::RegisterServerConfig()
	{
		m_config.RegisterStringOption("GameSettings.MapFile");
	}
}
