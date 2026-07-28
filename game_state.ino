#include "Train_HAS3_altar.h"

void SettingFunc()
{
    activate_bool = false;
    sendCommand("page pgSetting");
    NeoFunc = NeoNo;
    lightColor(pixels_round, white);
    // lightColor(pixels_side, white);
    lightColor(pixels_square, white);
}

void ReadyFunc()
{
    activate_bool = false;
    sendCommand("page pgInfo");
    NeoFunc = NeoBeforeTagger;
}

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

String JsonStringValue(JsonVariant value)
{
    const char *text = value.as<const char *>();
    return text ? String(text) : String("");
}

void LogStringChange(const char *name, const String &oldValue, const String &newValue)
{
    Serial.print("[DataChange] ");
    Serial.print(name);
    Serial.print(": ");
    Serial.print(oldValue.length() ? oldValue : "(empty)");
    Serial.print(" -> ");
    Serial.println(newValue.length() ? newValue : "(empty)");
}

void LogIntChange(const char *name, int oldValue, int newValue)
{
    Serial.print("[DataChange] ");
    Serial.print(name);
    Serial.print(": ");
    Serial.print(oldValue);
    Serial.print(" -> ");
    Serial.println(newValue);
}

void DataChange()
{
    if (!(const char *)my["device_name"])
    {
        Serial.println("[DataChange] no server data, skipped");
        return;
    }

    static StaticJsonDocument<1000> cur;

    String cmd;
    String oldGameState = JsonStringValue(cur["game_state"]);
    String newGameState = JsonStringValue(my["game_state"]);
    String oldDeviceState = JsonStringValue(cur["device_state"]);
    String newDeviceState = JsonStringValue(my["device_state"]);
    int oldBrightness = (int)cur["brightness"];
    int newBrightness = (int)my["brightness"];
    int oldTakenChip = (int)cur["taken_chip"];
    int newTakenChip = (int)my["taken_chip"];
    int oldMaxChip = (int)cur["max_chip"];
    int newMaxChip = (int)my["max_chip"];

    bool brightnessChanged = (newBrightness != oldBrightness);

    if (newGameState != oldGameState)
    {
        LogStringChange("game_state", oldGameState, newGameState);

        if (newGameState == "setting")
        {
            if (brightnessChanged) applyBrightness();
            SettingFunc();
        }
        else if (newGameState == "ready")
        {
            if (brightnessChanged) applyBrightness();
            ReadyFunc();
        }
        else if (newGameState == "activate")
        {
            if (brightnessChanged) applyBrightness();
            activate_bool = true;
        }
    }
    else if (brightnessChanged)
    {
        applyBrightness();
    }

    if (brightnessChanged)
    {
        LogIntChange("brightness", oldBrightness, newBrightness);
    }

    if (newDeviceState != oldDeviceState)
    {
        LogStringChange("device_state", oldDeviceState, newDeviceState);

        if (newDeviceState == "activate")
        {
            sendCommand("page pgChipCount");
            SyncChipCount();
            NeoFunc = NeoGaming;
        }
        else if (newDeviceState == "player_win")
        {
            sendCommand("page pgTaggerLose");
            NeoFunc = NeoLose;
        }
        else if (newDeviceState == "player_lose")
        {
            sendCommand("page pgTaggerWin");
            NeoFunc = NeoWin;
        }
        else if (newDeviceState == "blink")
        {
            EnterAltarBlinkState();
        }
        else if (newDeviceState == "github")
        {
            Serial.println("[DataChange] OTA check requested");
            esp_task_wdt_delete(NULL);
            ota.check();
            esp_task_wdt_add(NULL);
        }
    }

    if (newTakenChip != oldTakenChip)
    {
        LogIntChange("taken_chip", oldTakenChip, newTakenChip);

        cmd = "pgChipCount.vSacrificeChip.val=" + String(newTakenChip);
        sendCommand(cmd.c_str());
        SyncLanguage();

        if (newTakenChip >= 1 && newTakenChip <= 10)
        {
            int vid = newTakenChip * 2 + nextion_language;
            cmd = "play 0," + String(vid) + ",0";
            sendCommand(cmd.c_str());
        }
    }

    if (newMaxChip != oldMaxChip)
    {
        LogIntChange("max_chip", oldMaxChip, newMaxChip);
        cmd = "pgChipCount.vMaxChip.val=" + String(newMaxChip);
        sendCommand(cmd.c_str());
    }

    SyncLanguage();

    Serial.println("[DataChange] done");
    cur = my;
}
