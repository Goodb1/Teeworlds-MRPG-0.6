#include "scenario_world_manager.h"
#include <game/server/player.h>

void CScenarioWorldManager::UpdateScenarios()
{
	// is timeout pending
	if(m_PendingStart.m_Active && time_get() >= m_PendingStart.m_StartAt)
		TryFinalizePendingStart();

	// update scenario
	if(m_pScenario && !m_PendingStart.m_Active)
	{
		m_pScenario->Tick();
		if(!m_pScenario->IsRunning())
			m_pScenario.reset();
	}
}

void CScenarioWorldManager::RemoveClient(int ClientID)
{
	// remove from pending
	if(m_PendingStart.m_Active)
	{
		m_PendingStart.m_JoinedPlayers.erase(ClientID);
		m_PendingStart.m_DeclinedPlayers.erase(ClientID);
	}

	// remove from scenario
	if(m_pScenario)
		m_pScenario->RemoveParticipant(ClientID);
}

std::shared_ptr<WorldScenarioBase> CScenarioWorldManager::GetActiveScenarioByPlayer(int ClientID) const
{
	return m_pScenario && m_pScenario->IsRunning() && m_pScenario->GetParticipants().contains(ClientID) ? m_pScenario : nullptr;
}

void CScenarioWorldManager::TryFinalizePendingStart()
{
	// is already ending
	if(!m_PendingStart.m_Active)
		return;

	m_PendingStart.m_Active = false;

	// check valid scenario
	if(!m_pScenario)
		return;

	// add joined players to scenario participant
	for(const int joinedClientID : m_PendingStart.m_JoinedPlayers)
		m_pScenario->AddParticipant(joinedClientID);

	// cancel is empty
	if(m_pScenario->GetParticipants().empty())
	{
		m_pGS->ChatWorld(m_pScenario->GetWorldID(), "World scenario", "Cancelled: no participants joined.");
		m_pScenario.reset();
		return;
	}

	// start world scenario
	m_pScenario->Start();
	if(!m_pScenario->IsRunning())
	{
		m_pScenario.reset();
		return;
	}

	m_pGS->ChatWorld(m_pScenario->GetWorldID(), "World scenario", "Started: {} participant(s).", (int)m_PendingStart.m_JoinedPlayers.size());
}

void CScenarioWorldManager::AttachVoteForPlayer(int ClientID)
{
	// is already ending
	if(!m_PendingStart.m_Active)
		return;

	// check valid time for attach & valid
	if(!m_pScenario || time_get() >= m_PendingStart.m_StartAt)
		return;

	// check valid client world ID
	auto* pPlayer = m_pGS->GetPlayer(ClientID);
	if(!pPlayer || pPlayer->GetCurrentWorldID() != m_pScenario->GetWorldID())
		return;

	// register callback options vote
	const int remainingSeconds = std::max(1, (int)((m_PendingStart.m_StartAt - time_get()) / time_freq()));
	auto* pOption = CVoteOptional::Create(ClientID, remainingSeconds, "Join the scenario.");
	pOption->RegisterCallback([this](CPlayer* pVotePlayer, bool isJoined)
	{
		if(!m_PendingStart.m_Active || !pVotePlayer)
			return;

		const int playerCID = pVotePlayer->GetCID();
		m_PendingStart.m_JoinedPlayers.erase(playerCID);
		m_PendingStart.m_DeclinedPlayers.erase(playerCID);

		if(isJoined)
			m_PendingStart.m_JoinedPlayers.insert(playerCID);
		else
			m_PendingStart.m_DeclinedPlayers.insert(playerCID);
	});

	pOption->RegisterCloseCondition([this](CPlayer* pVotePlayer)
	{
		if(!m_PendingStart.m_Active || !pVotePlayer)
			return true;

		if(time_get() >= m_PendingStart.m_StartAt)
		{
			TryFinalizePendingStart();
			return true;
		}

		return !m_pScenario || pVotePlayer->GetCurrentWorldID() != m_pScenario->GetWorldID();
	});
}
