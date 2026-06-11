#include "Train_HAS3_altar.h"

//****************************************** Initialize ******************************************
void SensorInit()
{
  // Neopixel init
  pixels_square.begin();
  pixels_round.begin();
  pixels_side.begin();

  // Rfid init
  RfidInit();
}

//********************************************* Rfid *********************************************
/**
 * @brief RFID(=PN532) 세팅
 */
void RfidInit(void)
{
  nfc.begin(); // nfc 함수 시작
  if (!(nfc.getFirmwareVersion()))
  {
    Serial.println("!!!RFID 연결실패!!! - 계속 진행");
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "PN532");
    return;
  }
  nfc.SAMConfig(); // configure board to read RFID tags
  Serial.println("RFID 연결성공");
}

/**
 * @brief RFID 태그 인식
 */
void RfidLoop()
{
  if (!rfid_tag)
  {
    rfid_tag = true;
    rfid_timer_id = rfid_timer.setTimeout(1000, RfidTagTimerFunc);
  }
  else
  {
    return;
  }
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t data[32];
  char user_data[5];
  byte pn532_packetbuffer11[64];
  pn532_packetbuffer11[0] = 0x00;
  if (nfc.sendCommandCheckAck(pn532_packetbuffer11, 1))
  { // rfid 통신 가능한 상태인지 확인
    if (nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A))
    {                                    // rfid에 tag 찍혔는지 확인용 //데이터 들어오면 uid정보 가져오기
      if (nfc.ntag2xx_ReadPage(7, data)) // ntag 데이터에 접근해서 불러와서 data행열에 저장
        CardChecking(data);
    }
  }
}

/**
 * @brief 장치 이름(예: G9P1)에서 카드 역할을 고정 결정
 *        G9P1=술래, G9P2=유령, G9P3~G9P9=생존자
 */
String GetRoleFromName(const String &deviceName)
{
  // Temporary test card override.
  if (deviceName == "G2P2") return "tagger";
  if (!deviceName.startsWith("G9P")) return "unknown";
  int num = deviceName.substring(3).toInt();
  if (num == 1) return "tagger";
  if (num == 2) return "ghost";
  if (num >= 3 && num <= 9) return "survivor";
  return "unknown";
}

/**
 * @brief RFID에 태그된 NFC의 데이터에 따른 코드 동작
 *
 * @param rfidData 태그된 NFC의 데이터
 */
void CardChecking(uint8_t rfidData[32]) // 어떤 카드가 들어왔는지 확인용
{
  String tagUser = "";
  static String cur_tag_user = "";
  static uint32_t cur_tag_time = 0;
  for (int i = 0; i < 4; i++)
    tagUser += (char)rfidData[i];
  Serial.println("tag_user_data : " + tagUser);

  uint32_t now = millis();
  if (tagUser == cur_tag_user && now - cur_tag_time < 12000)
  {
    Serial.println("duplicate tag ignored : " + tagUser);
    return;
  }
  cur_tag_user = tagUser;
  cur_tag_time = now;

  // 1. 태그한 플레이어 정보 읽어오기
  has2wifi.Receive(tagUser);

  // 2. 장치 이름 기반으로 역할 고정 결정
  String tagRole = GetRoleFromName(tagUser);
  Serial.println("tag_role : " + tagRole);

  // 3. 술래 카드 태그 시 조건 없이 제단 활성화
  if (tagRole == "tagger")
  {
    NeoFunc = NeoNo;

    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
    delay(300);
    lightColor(pixels_round, purple);
    lightColor(pixels_side, purple);
    lightColor(pixels_square, purple);
    delay(300);
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
    delay(300);
    lightColor(pixels_round, purple);
    lightColor(pixels_side, purple);
    lightColor(pixels_square, purple);

    PlayAltarActivateVoice();
    delay(300);

    bool altarActivating = (String)(const char *)my["device_state"] != "activate";
    if (altarActivating)
    {
      has2wifi.Send((String)(const char *)my["device_name"], "game_state", "activate");
      has2wifi.Send((String)(const char *)my["device_name"], "device_state", "activate");
      sendCommand("page pgChipCount");
      NeoFunc = NeoGaming;

      String tagger_name = (String)(const char *)tag["device_name"];
      String tagger_group = tagger_name.substring(0, 2);
      for (int i = 1; i < 9; ++i)
      {
        String player_name = tagger_group + "P" + String(i);
        Serial.println(player_name);
        has2wifi.Send(player_name, "tagger_name", tagger_name);
      }
    }

  // 4. 술래 카드 태그 시 조건 없이 제단에 생명 바쳐짐
    for (int i = 0; i < NUMPIXELS_ROUND; i++)
    {
      if (i == 0)
      {
        pixels_side.clear();
        pixels_square.clear();
        pixels_round.clear();
      }
      lightColor(pixels_round, purple, i);
      delay(50);
    }

    has2wifi.Send((String)(const char *)my["device_name"], "taken_chip", "+1");

    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();

    NeoFunc = NeoGaming;
  }
}

bool RfidNsecTag(int sec)
{
  if (nsec_tag_num == 0 && !nsec_tag_bool)
  {
    nsec_tag_timer_id = nsec_tag_timer.setTimeout(5000, NsecTagTimerFailFunc);
    nsec_tag_bool = true;
  }
  else
  {
    nsec_tag_timer.restartTimer(nsec_tag_timer_id);
  }

  if (nsec_tag_num >= sec && nsec_tag_bool)
  {
    Serial.println("태그 성공");
    nsec_tag_timer.deleteTimer(nsec_tag_timer_id);
    nsec_tag_bool = false;
    nsec_tag_timer_id = nsec_tag_timer.setTimeout(2000, NsecTagTimerSuccessFunc);
    return true;
  }
  else
  {
    nsec_tag_num++;
  }
  return false;
}

//******************************************* Neopixel Helpers *******************************************
void applyBrightness()
{
  int b = (int)my["brightness"];
  int brightness;
  if (b <= 0 || b > 100)
    brightness = 255;
  else
    brightness = map(b, 0, 100, 0, 255);
  pixels_square.setBrightness(brightness);
  pixels_round.setBrightness(brightness);
  pixels_side.setBrightness(brightness);
  pixels_square.show();
  pixels_round.show();
  pixels_side.show();
}

void lightColor(Adafruit_NeoPixel &pixels, int color[3])
{
  pixels.fill(pixels.Color(color[0], color[1], color[2]));
  pixels.show();
}

void lightColor(Adafruit_NeoPixel &pixels, int color[3], int index)
{
  pixels.setPixelColor(index, color[0], color[1], color[2]);
  pixels.show();
}

void lightRgb(Adafruit_NeoPixel &pixels, int r, int g, int b)
{
  pixels.fill(pixels.Color(r, g, b));
  pixels.show();
}

//******************************************* Neopixel *******************************************
void NeoNo()
{
}
// A 상태
void NeoBeforeTagger()
{
  delay(100);
  static int breathe = 0;
  static bool breathe_direction = true;

  breathe_direction ? breathe++ : breathe--;

  lightColor(pixels_round, white);
  lightRgb(pixels_side, breathe, breathe, breathe);
  lightColor(pixels_square, red);

  if (breathe == 0)
  {
    breathe_direction = true;
  }
  else if (breathe == 20)
  {
    breathe_direction = false;
  }
}

void NeoTagger()
{
  delay(100);
  static int breathe_2 = 0;
  static bool breathe_direction_2 = true;

  breathe_direction_2 ? breathe_2++ : breathe_2--;

  lightRgb(pixels_side, breathe_2, breathe_2, breathe_2);

  if (breathe_2 == 0)
  {
    breathe_direction_2 = true;
  }
  else if (breathe_2 == 20)
  {
    breathe_direction_2 = false;
  }

  pixels_round.clear();
  NeoArrow();
}

void NeoTaggerTag()
{
  static int tag_neo = 0;

  pixels_round.clear();
  pixels_side.clear();

  lightColor(pixels_round, purple, tag_neo);

  if (++tag_neo > NUMPIXELS_ROUND)
  {
    tag_neo = 0;

    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
  }
}

void NeoAfterTagger()
{
  static bool after_tagger_neo_bool = false;

  if (after_tagger_neo_bool)
  {
    after_tagger_neo_bool = false;
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
  }
  else
  {
    after_tagger_neo_bool = true;
    lightColor(pixels_round, purple);
    lightColor(pixels_side, purple);
    lightColor(pixels_square, purple);
  }
}

void NeoGaming()
{
  delay(100);
  static int breathe = 0;
  static bool breathe_direction = true;

  breathe_direction ? breathe++ : breathe--;

  lightRgb(pixels_round, breathe, 0, breathe);
  lightRgb(pixels_side, breathe, 0, breathe);
  NeoArrow();

  if (breathe == 0)
  {
    breathe_direction = true;
  }
  else if (breathe == 20)
  {
    breathe_direction = false;
  }
}

// void NeoTakenChip()
// {
//   static int chip_neo = 0;

//   if(chip_neo == 0){
//     pixels_side.clear();
//     pixels_square.clear();
//     pixels_round.clear();
//   }

//   pixels_round.lightColor(purple, chip_neo);

//   if(++chip_neo > NUMPIXELS_ROUND){
//     chip_neo = 0;

//     pixels_round.clear();
//     pixels_side.clear();
//     pixels_square.clear();
//   }
// }

void NeoWin()
{
  static bool win_neo_bool = false;
  static int win_neo = 20;
  static int win_neo_delay = 1500;

  win_neo_delay = win_neo_delay - 100;

  if (win_neo_bool)
  {
    win_neo_bool = false;
    lightRgb(pixels_round, 0, 0, win_neo);
    lightRgb(pixels_side, 0, 0, win_neo);
    lightRgb(pixels_square, 0, 0, win_neo);
  }
  else
  {
    win_neo_bool = true;
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
  }

  if (win_neo_delay <= 300)
  {
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();

    NeoFunc = NeoNo;
  }
  delay(win_neo_delay);
}

void NeoLose()
{
  static bool lose_neo_bool = false;
  static int lose_neo = 20;
  static int lose_neo_delay = 1500;

  lose_neo_delay = lose_neo_delay - 100;

  // 깜빡임을 표현
  if (lose_neo_bool)
  {
    lose_neo_bool = false;
    lightRgb(pixels_round, lose_neo, 0, 0);
    lightRgb(pixels_side, lose_neo, 0, 0);
    lightRgb(pixels_square, lose_neo, 0, 0);
  }
  else
  {
    lose_neo_bool = true;
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();
  }

  if (lose_neo_delay <= 300)
  {
    pixels_round.clear();
    pixels_side.clear();
    pixels_square.clear();

    NeoFunc = NeoNo;
  }
  delay(lose_neo_delay);
}

void NeoArrow()
{
  static int arrow_pattern = 0;

  switch (arrow_pattern)
  {
  case 0:
    pixels_square.clear();
    break;

  case 1:
    arrow_neo_line_1 = 0;
    arrow_neo_line_2 = 16;
    arrow_neo_line_3 = 0;
    break;

  case 2:
    arrow_neo_line_1 = 1;
    arrow_neo_line_2 = 24;
    arrow_neo_line_3 = 1;
    break;

  case 3:
    arrow_neo_line_1 = 3;
    arrow_neo_line_2 = 12;
    arrow_neo_line_3 = 3;
    break;

  case 4:
    arrow_neo_line_1 = 6;
    arrow_neo_line_2 = 6;
    arrow_neo_line_3 = 6;
    break;

  case 5:
    arrow_neo_line_1 = 12;
    arrow_neo_line_2 = 3;
    arrow_neo_line_3 = 12;
    break;

  case 6:
    arrow_neo_line_1 = 24;
    arrow_neo_line_2 = 1;
    arrow_neo_line_3 = 24;
    break;

  case 7:
    arrow_neo_line_1 = 16;
    arrow_neo_line_2 = 0;
    arrow_neo_line_3 = 16;
    break;

  default:
    break;
  }

  if (++arrow_pattern > 7)
  {
    arrow_pattern = 0;
  }

  NeoArrowSet(1, arrow_neo_line_1);
  // NeoArrowSet(2, arrow_neo_line_2);
  NeoArrowSet(3, arrow_neo_line_3);
  pixels_square.show();
}

void NeoArrowSet(int arrow_neo_line_num, int arrow_neo_line)
{
  int neo_num = 0;

  if (arrow_neo_line_num == 1)
  {
    neo_num = 0;
  }
  else if (arrow_neo_line_num == 2)
  {
    neo_num = 5;
  }
  else if (arrow_neo_line_num == 3)
  {
    neo_num = 10;
  }

  switch (arrow_neo_line)
  {
  case 0:
    pixels_square.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 1:
    pixels_square.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 5, 20, 20, 20);
    break;
  case 3:
    pixels_square.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 4, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 5, 20, 20, 20);
    break;
  case 6:
    pixels_square.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 3, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 4, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 12:
    pixels_square.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 2, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 3, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 24:
    pixels_square.setPixelColor(neo_num + 1, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 2, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 16:
    pixels_square.setPixelColor(neo_num + 1, 20, 20, 20);
    pixels_square.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_square.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  default:
    break;
  }
}
