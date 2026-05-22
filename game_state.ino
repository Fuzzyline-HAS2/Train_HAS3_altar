#include "Train_HAS3_altar.h"

/**
 * @brief DB gamestate가 setting 일 때 한번동작하는 코드
 */
void SettingFunc()
{
    activate_bool = false;
    sendCommand("page pgSetting");
    NeoFunc = NeoNo;
    lightColor(pixels_round, white);
    lightColor(pixels_side, white);
    lightColor(pixels_square, white);
}

/**
 * @brief DB gamestate가 ready 일 때 한번동작하는 코드
 */

void ReadyFunc()
{
    activate_bool = false;
    sendCommand("page pgInfo");
    NeoFunc = NeoBeforeTagger;
}

/**
 * @brief DB gamestate가 activate 일 때 반복동작하는 코드
 */
void ActivateFunc()
{
    RfidLoop();
}

void EnterAltarBlinkState()
{
    sendCommand("page pgAltarTag");
    NeoFunc = NeoTagger;
    activate_bool = true;
}

void DataChange()
{
    if (!(const char *)my["device_name"])
    {
        Serial.println("[DataChange] 서버 데이터 없음, 스킵");
        return;
    }

    static StaticJsonDocument<2048> cur;

    String cmd;

    bool brightness_changed = ((int)my["brightness"] != (int)cur["brightness"]);

    if ((String)(const char *)my["game_state"] != (String)(const char *)cur["game_state"])
    {
        if ((String)(const char *)my["game_state"] == "setting")
        {
            if (brightness_changed) applyBrightness();
            SettingFunc();
            EnterAltarBlinkState();
            has2wifi.Send((String)(const char *)my["device_name"], "game_state", "activate");
            has2wifi.Send((String)(const char *)my["device_name"], "device_state", "blink");
        }
        else if ((String)(const char *)my["game_state"] == "ready")
        {
            if (brightness_changed) applyBrightness();
            ReadyFunc();
        }
        else if ((String)(const char *)my["game_state"] == "activate")
        {
            if (brightness_changed) applyBrightness();
            activate_bool = true;
        }
    }
    else if (brightness_changed)
    {
        applyBrightness();
    }

    if ((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"])
    {
        if ((String)(const char *)my["device_state"] == "activate")
        {
            sendCommand("page pgChipCount");
            NeoFunc = NeoGaming;
        }
        else if ((String)(const char *)my["device_state"] == "player_win")
        {
            sendCommand("page pgTaggerLose");
            NeoFunc = NeoLose;
        }
        else if ((String)(const char *)my["device_state"] == "player_lose")
        {
            sendCommand("page pgTaggerWin");
            NeoFunc = NeoWin;
        }
        else if ((String)(const char *)my["device_state"] == "blink")
        {
            EnterAltarBlinkState();
        }
        else if ((String)(const char *)my["device_state"] == "github")
        {
            ota.check();
        }
    }

    if((int)my["taken_chip"] != (int)cur["taken_chip"])
    {
        int takenChip = (int)my["taken_chip"];

        cmd = "pgChipCount.vSacrificeChip.val=" + String(takenChip);
        sendCommand(cmd.c_str());
        SyncLanguage();

        if (takenChip >= 1 && takenChip <= 10)
        {
            int vid = takenChip * 2 + nextion_language;
            cmd = "play 0," + String(vid) + ",0";
            sendCommand(cmd.c_str());
        }
    }
    if((int)my["max_taken_chip"] != (int)cur["max_taken_chip"])
    {
        cmd = "pgChipCount.vMaxChip.val=" + (String)(int)my["max_taken_chip"];
        sendCommand(cmd.c_str());
    }
    SyncLanguage();

    Serial.println("Data Change");
    cur = my;
}
