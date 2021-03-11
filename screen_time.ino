/*
 * システム概要：赤外線距離センサーで電子機器を使用しているか否かを判定し、一定時間以上使用している時警告するシステム
 */
#include "ESP8266.h"

// Wi-Fi SSID
#define SSID        "aterm-44eb6d-a"//ルーターのSSID
// Wi-Fi PASSWORD
#define PASSWORD    "5a709b9ca02ed"//ルーターのパスワード
// サーバーのホスト名
#define HOST_NAME   "kamake.co.jp"//KAMAKEサーバーのURL
// ポート番号
#define HOST_PORT   80
#define buttonON LOW//スイッチを押したときの宣言
#define C5 1046.502//C5の音を1046.50と定義
 
ESP8266 wifi(Serial1);

#define DIR_NAME      "/iot-seminar/20210205/0105"  //サーバーのパス
#define SENT_MESSAGE  "hoge"                        //受信内容
#define MAIL_ADDRESS  "mami.kikuchi2414@gmail.com"                 //メールアドレス

  // サーバーへ渡す情報
const char ledGet[] PROGMEM = "GET "DIR_NAME"/ledGet.php HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char logGet[] PROGMEM = "GET "DIR_NAME"/logGet.php HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char logSent[] PROGMEM = "GET "DIR_NAME"/logSent.php?message="SENT_MESSAGE" HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char mailSent[] PROGMEM = "GET "DIR_NAME"/mailSent.php?message="MAIL_ADDRESS" HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";

const char *const send_table[] PROGMEM = {
  logGet,logSent,mailSent//j=0の時文字受信、1の時文字送信、2の時メール送信
};

//millis関数：プログラム実行開始時から現在までの時間を
//ミリ秒単位で教えてくれるタイマー関数
unsigned long now_time;//現在の時間
unsigned long use_start_time;//使用時間計測開始
unsigned long use_time;//使用時間
unsigned long use_time_tmp;//使用時間の保持
unsigned long rest_start_time;//休憩時間計測開始
unsigned long rest_time;//休憩時間
unsigned long rest_time_tmp;//休憩時間の保持
unsigned long safe_start_time;//許容時間の計測開始
unsigned long safe;//許容時間の計測
unsigned long safe_time_tmp;//許容時間の計測開始

float Vcc = 5.0;
float dist1;
float dist2;
int count=0; //使用時間計測の回数
int count2=0; //休憩時間計測の回数
int count3=0;//許容時間計測の回数
int count4=0;//メール送信の制御

//_use:使用時間の分秒, _rest:休憩時間の分秒, _safe:許容時間の分秒
int min_use=0;
int sec_use=0;
int min_rest=0;
int sec_rest=0;
int min_safe=0;
int sec_safe=0;
/*状態用変数
 * 0 : センサーOFFかつ計測停止, 1 : センサーONかつ計測開始
 * 2 : センサーOFFかつ計測開始, 3 : センサーONかつ計測停止
 */
int state;
/*電子機器用変数
 * 0 : スマホ, 1 : パソコン
 * 2 : タブレット
 */
int dev;
/**
 * 初期設定
 */
void setup(void)
{

  state = 0;
  dev=0;
  use_start_time=0;
  rest_start_time=0;
  use_time=0;
  rest_time=0;
  use_time_tmp=0;
  rest_time_tmp=0;
  safe_start_time=0;
  safe=0;
  safe_time_tmp=0;
  
  pinMode(2,INPUT_PULLUP);//2番をスイッチ1の入力として設定 
  pinMode(13, OUTPUT);// デジタル13番ピンを出力として設定
  pinMode(11,OUTPUT);//11番をブザーの出力に設定
 Serial.begin(9600);
  while (1) {
    Serial.print("restaring esp8266...");
    if (wifi.restart()) {
      Serial.print("ok\r\n");
      break;
    }
    else {
      Serial.print("not ok...\r\n");
      Serial.print("Trying to kick...");
      while (1) {
        if (wifi.kick()) {
          Serial.print("ok\r\n");
          break;
        }
        else {
          Serial.print("not ok... Wait 5 sec and retry...\r\n");
          delay(5000);
        }
      }
    }
  }
  
  
  Serial.print("setup begin\r\n");

  Serial.print("FW Version:");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print("to station + softap ok\r\n");
  } else {
    Serial.print("to station + softap err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP:");
    Serial.println( wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }
 
  Serial.print("setup end\r\n");
}

//
// ループ処理
//
void loop(void)
{
  uint8_t buffer[340] = {0};//受信バッファ
  char sendStr[160];//送信バッファ
  static int j = 0;//サーバーアクセス情報の指定番号
  char min_text[5]="";//分
  char sec_text[5]="";//秒
  char text[15]="";//受信した文字列
  char message[160]="GET ";
  char message2[30]="/logSent.php?message=";
  char message3[30]=" HTTP/1.0\r\nHost:";
  char message4[40]=" kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
  
  //toCharArray
  //距離を算出
  dist1 = Vcc*analogRead(A0)/1023;//電圧
  dist2 = 26.549*pow(dist1,-1.2091);//距離dist2の近似式
  Serial.println("距離:"+String(dist2));
  //スイッチがオンの時
  if(digitalRead(2)==buttonON){
    if(state==0){
      
      state=1;
      
     
    }else if(state==2){
      state=3;
      digitalWrite(13, LOW);//LEDをオフ
      noTone(11);//11番ピンのブザーを鳴らさない
      count=0;
      count2=0;
      count3=0;
      count4=0;
      use_time = 0;
      rest_time = 0;
      safe = 0;
      use_time_tmp = 0;
      rest_time_tmp = 0;
      safe_time_tmp = 0;
      rest_start_time = 0;
      use_start_time = 0;
      safe_start_time = 0;
      
 
   }
  }else if(digitalRead(2)!=buttonON){
    if(state==1){
      state=2;
    }else if(state==3){
      state=0;
    }

    if(state==2){
       //現在の時間を計測
      now_time = millis();

      
      
      if((dev==0 && dist2<=30) || (dev==1 && dist2<=80) || (dev==2 && dist2<=50)){
          
         count2 = 0;

         rest_start_time = 0;
         
         
        
        //プログラム実行開始時から現在までの時間取得
        if(rest_time< 300000 && rest_time!=0){//許容時間が5分(300000)以下(0分を除く)の時
           if(count==0){//計測開始時の最初は計測時間を0にする
              use_time = use_time_tmp;
              count = 1;
           }else{
              use_time += now_time - use_start_time;
           }
              
              min_use = use_time / 60000;
              sec_use = (use_time % 60000) / 1000;
               //値の更新
               use_start_time = now_time;
              
          }else{
            if(count==0){//計測開始時の最初は計測時間を0にする
              use_time = 0;
              count = 1;
            }else{
              use_time += now_time - use_start_time;
            }
              min_use = use_time / 60000;
              sec_use = (use_time % 60000) / 1000;
               //値の更新
              use_start_time = now_time;
          }
          
          
          
          //使用時間が60分(3600000)以上かつ休憩時間が10分(600000)未満の時
          if(use_time>=3600000 && rest_time< 600000){
            digitalWrite(13, HIGH);//LEDをオン
            tone(11,C5);//11番ピンのブザーを鳴らす

            //メール送信
            if(count4 == 0){
              j = 2;
              count4=1;
            }
            
            //休憩時間を保持
            rest_time_tmp = rest_time;
            
            //許容時間の計測
            if(rest_time != 0){
              if(count3==0){//計測開始時の最初は計測時間を0にする
                safe = safe_time_tmp;
                count3 = 1;
              }else{
                safe += now_time - safe_start_time;
              }
              min_safe = safe / 60000;
              sec_safe = (safe % 60000) / 1000;
              
              safe_start_time =  now_time;
            }else{
              safe = 0;
              min_safe = safe / 60000;
              sec_safe = (safe % 60000) / 1000;
            }
          }
         
          
          
      }else{

          count = 0;
          count3 = 0;
          count4 = 0;

          use_start_time = 0;
          safe_start_time = 0;

          //使用時間を保持
          use_time_tmp = use_time;
          //許容時間を保持
          safe_time_tmp = safe;
          
          if(safe <= 300000 && safe!=0){//許容時間が5分(300000)以下(0分を除く)の時
           if(count2==0){//計測開始時の最初は計測時間を0にする
              rest_time = rest_time_tmp;
              count2 = 1;
           }else{
              rest_time += now_time -  rest_start_time;
           }
              min_rest = rest_time / 60000;
              sec_rest = (rest_time % 60000) / 1000;
              digitalWrite(13, LOW);//LEDをオフ
              noTone(11);//11番ピンのブザーを鳴らさない
              rest_start_time = now_time;
              
          }else{
            if(count2==0){//計測開始時の最初は計測時間を0にする
              rest_time = 0;
              count2 = 1;
            }else{
              rest_time += now_time -  rest_start_time;
           }
              min_rest = rest_time / 60000;
              sec_rest = (rest_time % 60000) / 1000;
              digitalWrite(13, LOW);//LEDをオフ
              noTone(11);//11番ピンのブザーを鳴らさない
              rest_start_time = now_time;
          }
          
        
          //休憩時間が10分(600000)以上の時
          if(rest_time>=600000){
            //使用時間と休憩時間をリセット
            use_time = 0;
            rest_time = 0;
            safe = 0;
            use_time_tmp = 0;
            rest_time_tmp = 0;
            safe_time_tmp = 0;
            rest_start_time = 0;
            use_start_time = 0;
            count = 0;
            safe_start_time = 0;
            count2 = 0;
            count3 = 0;
            count4 = 0;
          }
          
      }

    String(min_use).toCharArray(min_text,5);
    String(sec_use).toCharArray(sec_text,5);
    strcat(text,min_text);
    strcat(text,"分");
    strcat(text,sec_text);
    strcat(text,"秒");
    strcat(message,DIR_NAME);
    strcat(message,message2);
    strcat(message,text);
    strcat(message,message3);
    strcat(message,message4);
     
    Serial.println("使用時間："+String(min_use)+"分"+String(sec_use)+"秒");
    Serial.println("休憩時間："+String(min_rest)+"分"+String(sec_rest)+"秒");
    Serial.println("許容時間："+String(min_safe)+"分"+String(sec_safe)+"秒");
    Serial.println("use_start_time："+String(use_start_time));
    Serial.println("rest_start_time："+String(rest_start_time));
    Serial.println("safe_start_time："+String(safe_start_time));
    Serial.println("保持している使用時間："+String(use_time_tmp));
    Serial.println("保持している休憩時間："+String(rest_time_tmp));
    Serial.println("保持している許容時間："+String(safe_time_tmp));
    Serial.println("count："+String(count));
    Serial.println("count2："+String(count2));
    Serial.println("count3："+String(count3));
    Serial.println("count4："+String(count4));
    Serial.println(message);
    Serial.println("jの値:"+String(j));
    Serial.println("dev:"+String(dev));
    // TCPで接続
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
      Serial.print("create tcp ok\r\n");
    } else {
      Serial.print("create tcp err\r\n");
    }
  
    if(j==1){
      strcpy(sendStr, message);//サーバーアクセス情報を設定
    }else{
      strcpy_P(sendStr, (char *)pgm_read_word(&(send_table[j])));//サーバーアクセス情報を設定
    }
    Serial.println(sendStr);
  
    wifi.send((const uint8_t*)sendStr, strlen(sendStr));//Wi-Fi経由でアクセス情報をサーバーに送信
   
    //サーバからの文字列を入れるための変数
    String resultCode = "";
  
    // 取得した文字列の長さ(uint32_t:32ビットの非負整数を格納)
    uint32_t len = wifi.recv(buffer, sizeof(buffer), 10000);
  
    // 取得した文字数が0でなければ
    if (len > 0) {
      for(uint32_t i = 0; i < len; i++) {
        resultCode += (char)buffer[i];
      }
  
      // lastIndexOfでresultCodeの最後から改行を探す(位置を返す)
      int lastLF = resultCode.lastIndexOf('\n');
  
      // resultCodeの長さを求める
      int resultCodeLength = resultCode.length();
    
      // substringで改行コードの次の文字から最後までを求める
      //substring(from,to):from+1文字目からtoまでの文字列を返す
      String resultString = resultCode.substring(lastLF+1, resultCodeLength);
  
      // 前後のスペースを取り除く
      resultString.trim();
  
      Serial.print("resultString = ");
      Serial.println(resultString);
  //サーバーから端末名が送られる
    if(j == 0){
        // 取得した文字列がスマホならば
      if(resultString == "スマホ" || resultString == "スマートフォン") {
       dev = 0;
     } else if(resultString == "パソコン" || resultString == "PC" || resultString == "pc"){
       dev = 1;
      }else if(resultString == "タブレット"){
        dev = 2;
      }else{
        dev = 0;
      }
     }
    }
  
    
   if(j < 1){
      j = j+1;
    }
    else{
      j = 0;
   }
    }
  }
  //stateの表示
  Serial.println("State"+String(state));
  // 1秒待つ
  delay(1000);
}
