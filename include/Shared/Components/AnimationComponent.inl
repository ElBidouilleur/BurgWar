// Copyright (C) 2018 Jérôme Leclercq
// This file is part of the "Erewhon Server" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Shared/Components/AnimationComponent.hpp>

namespace bw
{
	inline AnimationComponent::AnimationComponent(std::shared_ptr<AnimationStore> animStore) :
	m_animationStore(std::move(animStore))
	{
	}

	inline AnimationComponent::AnimationComponent(const AnimationComponent& animation) :
	m_animationStore(animation.m_animationStore),
	m_playingAnimation(animation.m_playingAnimation)
	{
	}

	inline std::size_t AnimationComponent::GetAnimId() const
	{
		assert(IsPlaying());
		return m_playingAnimation->animId;
	}

	inline const std::shared_ptr<AnimationStore>& AnimationComponent::GetAnimationStore() const
	{
		return m_animationStore;
	}

	inline Nz::UInt64 AnimationComponent::GetEndTime() const
	{
		assert(IsPlaying());
		return m_playingAnimation->endTime;
	}

	inline Nz::UInt64 AnimationComponent::GetStartTime() const
	{
		assert(IsPlaying());
		return m_playingAnimation->startTime;
	}

	inline bool AnimationComponent::IsPlaying() const
	{
		return m_playingAnimation.has_value();
	}
}
