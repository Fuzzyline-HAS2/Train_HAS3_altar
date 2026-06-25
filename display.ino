#include "Train_HAS3_altar.h"

/**
 * @brief 디스플레이에 변화를 주거나 변화가 있을시 실행
 */
void DisplayCheck()
{
    while (Serial2.available() > 0)
    {
        String nextion_string = Serial2.readStringUntil(' ');
        NextionReceived(&nextion_string);
    }
}

/**
 * @brief 디스플레이에서 오는 Serial을 확인
 *
 * @param nextion_string Serial 문자열 데이터
 */
void NextionReceived(String *nextion_string)
{
}

/**
 * @brief 제단 활성화 시 Nextion 음성 출력
 *        Nextion 네이티브 play 명령으로 "제단이 활성화되었습니다." 음성 재생
 *        오디오 리소스 ID: 36=EN, 37=KO (nextion_language: 0=EN, 1=KO)
 *        (변수 vAudioPlay.val=1 방식은 Variable 컴포넌트에 값변경 이벤트가
 *         없어 재생이 트리거되지 않으므로 play 명령으로 변경)
 */
void PlayAltarActivateVoice()
{
    Serial.println("[Nextion] PlayAltarActivateVoice");
    int vid = 36 + nextion_language;   // 36=EN, 37=KO
    String cmd = "play 0," + String(vid) + ",0";
    sendCommand(cmd.c_str());
}

/**
 * @brief pgChipCount 페이지의 칩 수치(현재/최대)를 서버값으로 강제 동기화
 *
 *        DataChange 의 동기화는 값이 "바뀔 때만"(change detection) 전송하므로,
 *        값은 그대로인 채 pgChipCount 페이지로 새로 진입하면 Nextion HMI 의
 *        설계상 기본값(최대 생명 10 등)이 그대로 보이게 된다.
 *        페이지에 진입할 때마다 이 함수를 호출해 현재 서버값
 *        (max_chip, taken_chip)을 다시 써주면 최대 생명 개수가
 *        서버 설정대로 가변 표시된다.
 */
void SyncChipCount()
{
    String cmd;
    cmd = "pgChipCount.vMaxChip.val=" + (String)(int)my["max_chip"];
    sendCommand(cmd.c_str());
    cmd = "pgChipCount.vSacrificeChip.val=" + (String)(int)my["taken_chip"];
    sendCommand(cmd.c_str());
}

/**
 * @brief get 명령으로 Nextion 숫자 변수를 읽음
 *        성공시 값 반환, 실패시 -1 반환 (0x71 응답 파싱)
 */
int NextionGetNum(const char *cmd)
{
    while (Serial2.available()) Serial2.read();  // 이전 잔여 데이터 제거
    sendCommand(cmd);

    uint8_t buf[8] = {0};
    int idx = 0;
    uint32_t timeout = millis() + 500;
    while (millis() < timeout && idx < 8)
    {
        if (Serial2.available())
            buf[idx++] = Serial2.read();
    }

    // Nextion 숫자 응답: 0x71 + 4바이트(리틀엔디안) + 0xFF 0xFF 0xFF
    if (idx >= 5 && buf[0] == 0x71)
        return (int32_t)(buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24));

    return -1;
}

/**
 * @brief 서버 selected_language 와 Nextion vLanguage 를 동기화
 *        값이 다를 때만 Nextion 으로 전송 (0=EN, 1=KO)
 */
void SyncLanguage()
{
    int target = ((String)(const char *)shift_machine["selected_language"] == "EN") ? 0 : 1;
    if (nextion_language != target)
    {
        String cmd = "pgSetting.vLanguage.val=" + String(target);
        sendCommand(cmd.c_str());
        nextion_language = target;
    }
}

/**
 * @brief 초기 연결 후 Nextion 에서 언어/버전 값을 읽어 초기화
 *        - vLanguage: ESP 내부 저장 후 서버와 동기화
 *        - vVersion: 서버 nextion_version 필드에 전송
 */
void NextionInit()
{
    delay(200);

    int lang = NextionGetNum("get pgSetting.vLanguage.val");
    nextion_language = (lang >= 0) ? lang : 1;  // 읽기 실패시 기본값 KO(1)

    int version = NextionGetNum("get pgSetting.vVersion.val");
    if (version >= 0 && (const char *)my["device_name"])
    {
        has2wifi.Send((String)(const char *)my["device_name"],"nextion_version",String(version));
    }

    SyncLanguage();
}
