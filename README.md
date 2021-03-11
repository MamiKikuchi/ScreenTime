# ScreenTime
Arduinoを用いた電子機器の画面の見過ぎを警告するシステム

対象ユーザー：リモートワーク(オンライン授業)で電子機器を見る事が増えた人、スマホ(パソコン)依存症の人

使用部品：	赤外線距離センサ―、LED(基板上)、ブザー、Wi-Fiモジュール、レベル変換、モジュール、スイッチ(1)、3端子レギュレータ、ブレッドボード(2)、Arduino Mega

システム説明
	電子機器の画面に赤外距離センサ―をつけ、一定距離を計測している状態を「画面を見ている」と定義する。その距離を計測している時間を計測し、一定時間以上になった時、警告としてLED点灯とブザーが鳴り、画面の見過ぎをメールで知らせる。また電子機器の種類をブラウザ上で選択することで、それぞれの機器に対応した判定を行える。また計測時間をスマホからも見れるようにする。

