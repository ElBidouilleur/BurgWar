// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_CORELIB_LOGSYSTEM_MATCHLOGGER_HPP
#define BURGWAR_CORELIB_LOGSYSTEM_MATCHLOGGER_HPP

#include <CoreLib/Export.hpp>
#include <CoreLib/LogSystem/Logger.hpp>
#include <CoreLib/LogSystem/MatchLogContext.hpp>

namespace bw
{
	class BURGWAR_CORELIB_API MatchLogger : public Logger
	{
		public:
			inline MatchLogger(BurgAppComponent& app, SharedMatch& sharedMatch, LogSide logSide);
			inline MatchLogger(BurgAppComponent& app, SharedMatch& sharedMatch, LogSide logSide, const AbstractLogger& logParent);
			MatchLogger(const MatchLogger&) = delete;
			MatchLogger(MatchLogger&&) noexcept = default;
			~MatchLogger() = default;

			bool ShouldLog(const LogContext& context) const override;

		private:
			void InitializeContext(LogContext& context) const override;
			LogContext* NewContext() const override;
			void OverrideContent(const LogContext& context, std::string& content) const override;

			SharedMatch& m_sharedMatch;
	};
}

#include <CoreLib/LogSystem/MatchLogger.inl>

#endif
