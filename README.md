# LED_Blink_through_Bluetooth_Serial
## 概要
Bluetoothによるシリアル通信は10 mほどの距離で通信可能な2.4 GHz帯の電波による無線通信である。

BluetoothによるWindows10とのシリアル通信では仮想ポートによって行われるため、通常の有線シリアル通信とほぼ同様の方法で通信が可能である。ESP32とWindows10間でBluetoothによるシリアル通信を行う方法を確認する。

## 環境
ESP32側のプログラムはArduino IDEにより作成する。

Windows10側のプログラムはC++/OpenSiv3Dにより作成する。

## 実装内容
Window10のGUIにより遠隔で遠隔にLチカを行う。

構成は以下の通りである。
- アプリ
  - Port選択シーン
  - Lチカシーン
    - オン/オフを手動で切り替える手動モード
    - 指定した時間でオン/オフを繰り返す点滅モード
