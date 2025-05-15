#include "TalentButton.h"
#include "Config.h"
#include "Log.h"
#include "Chat.h"

bool TalentButton_Enabled = false;

std::vector<std::string> TalentButtonServerScript::GetChunks(std::string s, uint8_t chunkSize)
{
    std::vector<std::string> chunks;
    for (uint32_t i = 0; i < s.size(); i += chunkSize)
        chunks.push_back(s.substr(i, chunkSize));
    return chunks;
}

void TalentButtonServerScript::SendChunkedPayload(Warden* warden, WardenPayloadMgr* payloadMgr, std::string payload, uint32 chunkSize)
{
    auto chunks = GetChunks(payload, chunkSize);

    if (!payloadMgr->GetPayloadById(_prePayloadId))
        payloadMgr->RegisterPayload(_prePayload, _prePayloadId);

    payloadMgr->QueuePayload(_prePayloadId);
    warden->ForceChecks();

    for (auto const& chunk : chunks)
    {
        std::string escapedChunk = chunk;
        size_t pos = 0;
        while ((pos = escapedChunk.find("]]", pos)) != std::string::npos)
            escapedChunk.replace(pos, 2, "] ]");

        auto smallPayload = "wlbuf = wlbuf .. [[" + escapedChunk + "]];";

        payloadMgr->RegisterPayload(smallPayload, _tmpPayloadId, true);
        payloadMgr->QueuePayload(_tmpPayloadId);
        warden->ForceChecks();
    }

    if (!payloadMgr->GetPayloadById(_postPayloadId))
        payloadMgr->RegisterPayload(_postPayload, _postPayloadId);

    payloadMgr->QueuePayload(_postPayloadId);
    warden->ForceChecks();
}

bool TalentButtonServerScript::CanPacketSend(WorldSession* session, WorldPacket& packet)
{
    if (!TalentButton_Enabled)
        return true;

    if (packet.GetOpcode() != SMSG_LOGIN_VERIFY_WORLD)
        return true;

    WardenWin* warden = (WardenWin*)session->GetWarden();
    if (!warden || !warden->IsInitialized())
        return true;

    auto payloadMgr = warden->GetPayloadMgr();
    if (!payloadMgr)
        return true;

    payloadMgr->ClearQueuedPayloads();
    payloadMgr->UnregisterPayload(_prePayloadId);
    payloadMgr->UnregisterPayload(_postPayloadId);
    payloadMgr->UnregisterPayload(_tmpPayloadId);

    SendChunkedPayload(warden, payloadMgr, _midPayload, 128);

    return true;
}

void TalentButtonWorldScript::OnAfterConfigLoad(bool /*reload*/)
{
    TalentButton_Enabled = sConfigMgr->GetOption<bool>("TalentButton.Enable", false);
}

void TalentButtonLevelUp::OnPlayerLevelChanged(Player* player, uint8 oldLevel)
{
    if (player->GetLevel() == 10 && oldLevel < 10)
    {
        uint32 spellId = 63624;
        uint32 achievementlId = 2716;
        const AchievementEntry* achievement = sAchievementStore.LookupEntry(achievementlId);

        if (!player->HasSpell(spellId))
            player->CastSpell(player, spellId, true);

        if (achievement)
            player->CompletedAchievement(achievement);
    }
}

Acore::ChatCommands::ChatCommandTable TalentButtonCommandScript::GetCommands() const
{
    static Acore::ChatCommands::ChatCommandTable commandTable =
    {
        { "modtalentreset", HandleModTalentResetCommand, SEC_PLAYER, Acore::ChatCommands::Console::Yes }
    };
    return commandTable;
}

bool TalentButtonCommandScript::HandleModTalentResetCommand(ChatHandler* handler, Optional<Acore::ChatCommands::PlayerIdentifier> target)
{
    Player* targetPlayer = nullptr;

    if (target)
    {
        targetPlayer = target->GetConnectedPlayer();
        if (targetPlayer != handler->GetSession()->GetPlayer())
        {
            return false;
        }
    }
    else
    {
        targetPlayer = handler->GetSession()->GetPlayer();
    }

    if (!CanUseCommand(targetPlayer, handler))
        return false;

    targetPlayer->resetTalents(true);
    targetPlayer->SendTalentsInfoData(false);
    handler->PSendSysMessage("Your talents have been reset.");

    return true;
}

bool TalentButtonCommandScript::CanUseCommand(Player* player, ChatHandler* handler)
{
    if (!sConfigMgr->GetOption<bool>("TalentButton.Enable", false) || player->GetLevel() < 10)
    {
        // No further checks are necessary because chat messages can only be sent while the player is alive.
        return false;
    }
    return true;
}

void AddTalentButtonScripts()
{
    new TalentButtonServerScript();
    new TalentButtonWorldScript();
    new TalentButtonLevelUp();
    new TalentButtonCommandScript();
}
