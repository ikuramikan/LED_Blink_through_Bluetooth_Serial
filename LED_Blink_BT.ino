#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define PIN_NUM 22 // LEDに接続するpin番号

// Bluetoothシリアル通信用クラス
BluetoothSerial SerialBT;

// マルチスレッド用タスクハンドル格納
// 要素0がTask_manual、要素1がTask_blink用とする
TaskHandle_t thp[2];

// モード（手動モード0か点滅モード1か）を表す変数
bool mode_LED_blink = 0;

// 手動モードにおけるLEDがONかOFFかを表す変数
bool LED_on = false;

// 点滅モードにおける点滅間隔を表す変数
uint8_t time_delay = 100;

void setup()
{
  // デバック用　後で消す
  Serial.begin(9600);
  
  // Bluetoothシリアル通信の開始
  SerialBT.begin("BS_LED");

  // このpinを使ってLチカする
  pinMode(PIN_NUM, OUTPUT);

  // Task_manualの発行、コア１で行う（コア０はbluetoothで使っているのでコア１の方がいいと思った）
  xTaskCreateUniversal(Task_manual, "Task_manual", 4096, NULL, 3, &thp[0], 1);
}

void loop()
{
  // 適当な時間遅延
  delay(200);

  // 1 Byte目の読み取り
  if(SerialBT.available())
  {
    // 1 Byte目の読み取り
    const uint8_t first_byte = SerialBT.read();
  
    // 1 Byte目の1 bit目をチェックし1だった、かつ2bit目が0だった場合、モードは手動モードである  
    if((first_byte >> 6) == 0b00000010)
    {
      // 点滅モードであった場合、モードを切り替える
      if(mode_LED_blink==1)
      {
        // Task_blinkを終了
        vTaskDelete(thp[1]);

        // LEDはOFFしておく
        digitalWrite(PIN_NUM, LOW);
  
        // モードを表す変数の変更
        mode_LED_blink = 0;
        
        // Task_manualの発行、コア１で行う（コア０はbluetoothで使っているのでコア１の方がいいと思った）
        xTaskCreateUniversal(Task_manual, "Task_manual", 4096, NULL, 3, &thp[0], 1);
      }
      
      // 手動モードの場合、3 bit目が0ならオフ、1ならオンである
      if(((first_byte&0b00100000) >> 5) == 1)
      {
        LED_on = true;
      }
      else
      {
        LED_on = false;
      }
    }
    // 1 Byte目の1 bit目をチェックし1だった、かつ2 bit目が1だった場合、モードは点滅モードである  
    else if((first_byte >> 7) == 1)
    {
      // 手動モードであった場合、モードを切り替える
      if(mode_LED_blink == 0)
      {
        // Task_manualの終了
        vTaskDelete(thp[0]);
        
        // LEDはOFFしておく
        LED_on = false;
        digitalWrite(PIN_NUM, LOW);
        
        // モードを表す変数の変更
        mode_LED_blink = 1;

        // Task_blinkの発行
        xTaskCreateUniversal(Task_blink, "Task_blink", 4096, NULL, 3, &thp[1], 1);
      }

      // 点滅モードの場合次のByteが点滅の間隔の程度を表す
      // 2 Byte目を読み取る
      if(SerialBT.available())
      {
        const uint8_t second_byte = SerialBT.read();
        time_delay = second_byte;
      }
      // 点滅モードなのに2 Byte目がなかった場合（万が一の場合）、点滅の間隔は変更しない）
      else
      {}
    }
    // EXITされた場合(Windows側がPORT選択シーンに戻った場合)
    else if(first_byte == 0)
    { 
      // もしTask_blinkが発行されていたら終了しTask_manualを発行する
      if(mode_LED_blink==1)
      {
        vTaskDelete(thp[1]);
        xTaskCreateUniversal(Task_manual, "Task_manual", 4096, NULL, 3, &thp[0], 1);

        mode_LED_blink=0;
      }

      // LEDをOFFする
      LED_on = false;
      digitalWrite(PIN_NUM, LOW);
    }
  }
}

// 手動モードのタスク
void Task_manual(void *args)
{
  while(1)
  {
    // LED_onによりLEDをONするかOFFする
    if(LED_on)
    {
      digitalWrite(PIN_NUM, HIGH);
    }
    else
    {
      digitalWrite(PIN_NUM, LOW);
    }
    
    // とりあえず入れておく遅延
    delay(200);
  }
}

// 点滅モードのタスク
void Task_blink(void *args)
{
  while(1)
  {
    // LEDを点滅させる
    digitalWrite(PIN_NUM, HIGH);
    delay(time_delay*10);

    // とりあえず入れておく遅延
    delay(1);
    
    digitalWrite(PIN_NUM, LOW);
    delay(time_delay*10);

    // とりあえず入れておく遅延(バランスのため)
    delay(1);
  }
}