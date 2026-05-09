#ifndef GAME_SERVER_CORE_TOOLS_EFFECT_MANAGER_H
#define GAME_SERVER_CORE_TOOLS_EFFECT_MANAGER_H

// effect typename
using EffectName = std::string;

// default programmed effects
inline const EffectName EFFECT_NAME_SLOWNESS { "Slowness" };
inline const EffectName EFFECT_NAME_STUN { "Stun" };
inline const EffectName EFFECT_NAME_POISON { "Poison" };
inline const EffectName EFFECT_NAME_LAST_STAND { "LastStand" };
inline const EffectName EFFECT_NAME_FIRE { "Fire" };

// effect manager
class CEffectManager
{
	std::unordered_map<EffectName, int> m_vmEffects;

public:
	bool Add(const EffectName& Effect, const int Ticks, const float Chance = 100.f)
	{
		if(Effect.empty())
			return false;

		if(Chance < 100.0f && random_float(100.0f) >= Chance)
			return false;

		m_vmEffects[Effect] = Ticks;
		return true;
	}

	bool Remove(const EffectName& Effect)
	{
		return m_vmEffects.erase(Effect) > 0;
	}

	bool RemoveAll()
	{
		if(m_vmEffects.empty())
			return false;

		m_vmEffects.clear();
		return true;
	}

	bool IsActive(const EffectName& Effect) const
	{
		return m_vmEffects.contains(Effect);
	}

	void PostTick()
	{
		for(auto it = m_vmEffects.begin(); it != m_vmEffects.end();)
		{
			if(--it->second <= 0)
				it = m_vmEffects.erase(it);
			else
				++it;
		}
	}
};

#endif
