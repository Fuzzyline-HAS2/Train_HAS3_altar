#include "Train_HAS3_altar.h"

void TimerInit()
{
  wifi_timer_id = wifi_timer.setInterval(2000, WifiTimerFunc);
}
/**
 * @brief 타이머 동작
 */
void TimerRun()
{
  nsec_tag_timer.run();
  wifi_timer.run();
}

void WifiTimerFunc()
{
  has2wifi.Loop(DataChange);
}

void NsecTagTimerFailFunc()
{
  Serial.println("태그 실패");
  nsec_tag_num = 0;
  nsec_tag_bool = false;
}

void NsecTagTimerSuccessFunc()
{
  Serial.println("태그 성공 후 2초");
  nsec_tag_num = 0;
}