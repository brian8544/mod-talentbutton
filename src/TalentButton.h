#ifndef MODULE_TALENTBUTTON_H
#define MODULE_TALENTBUTTON_H

#include "Player.h"
#include "ScriptMgr.h"
#include "WardenWin.h"
#include "Config.h"
#include "Chat.h"

extern bool TalentButton_Enabled;

// This is a very dirty way, since addon messages exist for server<->client communication.
const std::string _addonPayloadTalentButton = R"lua(
local function CreateTalentButton()
    local parent = _G["PlayerSpecTab2"]
    if not parent then
        print("Parent PlayerSpecTab2 not found!")
        return
    end

    if _G["TalentButton"] then return end

    local button = CreateFrame("CheckButton", "TalentButton", parent:GetParent(), "PlayerSpecTabTemplate")
    button:SetSize(32, 32)
    button:SetPoint("TOPLEFT", parent, "BOTTOMLEFT", 0, -22)

    local normalTexture = button:GetNormalTexture()
    normalTexture:SetTexture("Interface\\Icons\\Inv_misc_book_10")
    normalTexture:SetAllPoints()

    local backgroundTexture = _G[button:GetName() .. "Background"]
    backgroundTexture:SetTexture("Interface\\SpellBook\\SpellBook-SkillLineTab")
    backgroundTexture:SetSize(64, 64)
    backgroundTexture:SetPoint("TOPLEFT", button, "TOPLEFT", -3, 11)

    local checkedTexture = button:GetCheckedTexture()
    checkedTexture:SetTexture("Interface\\Buttons\\CheckButtonHilight")
    checkedTexture:SetAllPoints()

    local highlightTexture = button:GetHighlightTexture()
    highlightTexture:SetTexture("Interface\\Buttons\\ButtonHilight-Square")
    highlightTexture:SetAllPoints()

    button:SetScript("OnMouseDown", function(self) self:GetCheckedTexture():SetAlpha(1) end)
    button:SetScript("OnMouseUp", function(self) self:GetCheckedTexture():SetAlpha(0) end)
    button:SetScript("OnEnter", function(self) 
        self:GetHighlightTexture():SetAlpha(1)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText("Reset Talents", 1, 1, 1)
        GameTooltip:AddLine("Click to reset your talents.", 1, 1, 1)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function(self) 
        self:GetHighlightTexture():SetAlpha(0)
        GameTooltip:Hide()
    end)

    StaticPopupDialogs["RESET_TALENTS_CONFIRM"] = {
        text = "Are you sure you want to reset your talents?",
        button1 = "Yes",
        button2 = "No",
        OnAccept = function() SendChatMessage(".modtalentreset", "SAY") end,
        timeout = 0,
        whileDead = true,
        hideOnEscape = true,
        preferredIndex = 3
    }

    button:SetScript("OnClick", function() StaticPopup_Show("RESET_TALENTS_CONFIRM") end)
    button:Show()
end

local f = CreateFrame("Frame")
f:RegisterEvent("ADDON_LOADED")
f:SetScript("OnEvent", function(self, event, addonName)
    if addonName == "Blizzard_TalentUI" then
        CreateTalentButton()
        f:UnregisterEvent("ADDON_LOADED")
    end
end)
)lua";

const std::string _prePayload = "wlbuf = '';";
const std::string _postPayload = "local a,b=loadstring(wlbuf) if not a then message(b) else a() end";
const std::string _midPayload = _addonPayloadTalentButton;

const uint16 _prePayloadId = 9603;
const uint16 _postPayloadId = 9604;
const uint16 _tmpPayloadId = 9605;

class TalentButtonServerScript : public ServerScript
{
public:
    TalentButtonServerScript() : ServerScript("TalentButtonServerScript", { SERVERHOOK_CAN_PACKET_SEND }) {}

private:
    bool CanPacketSend(WorldSession* session, WorldPacket& packet) override;
    std::vector<std::string> GetChunks(std::string s, uint8_t chunkSize);
    void SendChunkedPayload(Warden* warden, WardenPayloadMgr* payloadMgr, std::string payload, uint32 chunkSize);
};

class TalentButtonWorldScript : public WorldScript
{
public:
    TalentButtonWorldScript() : WorldScript("TalentButtonWorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) {}

private:
    void OnAfterConfigLoad(bool reload) override;
};

class TalentButtonLevelUp : public PlayerScript
{
public:
    TalentButtonLevelUp() : PlayerScript("TalentButtonLevelUp") {}

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override;
};

class TalentButtonCommandScript : public CommandScript
{
public:
    TalentButtonCommandScript() : CommandScript("TalentButtonCommandScript") {}

    Acore::ChatCommands::ChatCommandTable GetCommands() const override;
    static bool HandleModTalentResetCommand(ChatHandler* handler, Optional<Acore::ChatCommands::PlayerIdentifier> target);
    static bool CanUseCommand(Player* player, ChatHandler* handler);
};

#endif // MODULE_TALENTBUTTON_H
