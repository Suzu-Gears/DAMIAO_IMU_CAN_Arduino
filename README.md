# DAMIAO_IMU_CAN_Arduino

[![Arduino Lint](https://github.com/Suzu-Gears/DAMIAO_IMU_CAN_Arduino/actions/workflows/lint.yml/badge.svg)](https://github.com/Suzu-Gears/DAMIAO_IMU_CAN_Arduino/actions/workflows/lint.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Suzu-Gears/DAMIAO_IMU_CAN_Arduino)

## 概要

このライブラリは、CAN通信を介してDAMIAO製6軸IMU（慣性計測ユニット）DM-IMU-L1をArduinoで利用するためのものです。センサーデータの読み取りや、各種パラメータの設定が可能です。

## 対応ハードウェアと依存ライブラリ

このライブラリは、`arduino::HardwareCAN`に準拠しています。CAN通信を行うための別途ライブラリが必要です。使用するボードに合わせて、適切なCANライブラリをインストールしてください。

|マイコンボード|CANライブラリ|
|---|---|
|Arduino (Uno R4 WiFi / Uno R4 Minima / Nano R4)|Arduino_CAN (ビルトイン)|
|Raspberry Pi Pico (RP2040/RP2350)|[RP2040PIO_CAN](https://github.com/eyr1n/RP2040PIO_CAN)|
|ESP32|[ESP32_TWAI](https://github.com/eyr1n/ESP32_TWAI)|

**注意:** CANトランシーバーモジュールが別途必要です。

## CAN通信IDについて

このライブラリは、IMUとのCAN通信において、MasterIDとSlaveIDを使用します。
- **SlaveID**: マイコン（マスター）がIMU（スレーブ）に対して指令を送る際に使用するCAN IDです。マイコンは、このSlaveID宛てにコマンドを送信します。
- **MasterID**: IMU（スレーブ）がマイコン（マスター）に対してデータや応答を返す際に使用するCAN IDです。IMUは、このMasterID宛てにデータを送信します。

このライブラリを使用する際は、IMUのMasterIDとSlaveIDに合わせてインスタンスを初期化する必要があります。

MasterIDとSlaveIDは、GUIで変更できる他、`setCANSlaveId()`および`setCANMasterID()`によって変更が可能です。（設定の永続化は下を参照）

## パラメータの保存について

DM-IMU-L1の各種設定（MasterID, SlaveID, 動作モードなど）は、工場出荷時のデフォルト値を除き、RAM上のみで有効です。電源を切ると失われます。
設定を永続化するには、IMUに「設定保存」コマンド`saveParameters()`を送信する必要があります。このコマンドを使用すると、現在の設定がIMUの内部メモリに書き込まれ、電源オフ後も保持されます。

**注意点:** IMUの内部メモリには書き込み回数に上限があります。頻繁に設定保存コマンドを実行すると、IMUのメモリ寿命を縮める可能性があります。必要な場合のみ保存コマンドを使用し、連続した書き込みは避けてください。


## インストール

<!--
### Arduino IDE ライブラリマネージャー

1.  Arduino IDE を開きます。
2.  `スケッチ > ライブラリをインクルード > ライブラリを管理...` に移動します。
3.  "DAMIAO_IMU_CAN" を検索し、最新バージョンをインストールします。
-->

### 手動インストール

#### Arduino IDE の「.ZIPライブラリをインポート」を使用（推奨）

1.  [GitHubリポジトリの最新リリース](https://github.com/Suzu-Gears/DAMIAO_IMU_CAN_Arduino/releases/latest)を `.zip` ファイルでダウンロードします。
2.  Arduino IDE で、`スケッチ > ライブラリをインクルード > .ZIPライブラリをインポート...` に移動します。
3.  ダウンロードした `.zip` ファイルを選択します。
4.  Arduino IDE を再起動します。

#### 直接配置

1.  [GitHubリポジトリの最新リリース](https://github.com/Suzu-Gears/DAMIAO_IMU_CAN_Arduino/releases/latest)を `.zip` ファイルでダウンロードします。
2.  ダウンロードしたファイルを解凍し、フォルダをArduinoのライブラリディレクトリ（例: `~/Documents/Arduino/libraries/`）に配置します。
3.  Arduino IDE を再起動します。

## 基本的な使い方

基本的な使用方法や詳細なコードについては、`examples` フォルダ内のサンプルスケッチを参照してください。

### 動作モードについて

DM-IMU-L1には、データの送信方法が異なる2つの動作モードがあります。

#### 1. リクエストモード (Request Mode)

マイコン側からデータ送信を要求（リクエスト）すると、IMUが1回だけデータを返信するモードです。IMUが常時CANメッセージを送信しないため、CANバスを圧迫しません。リクエストとデータの受信更新には、以下の2通りの方法があります。

-   **`requestXXXX()` と `update()` を組み合わせる方法 (ノンブロッキング)**

    `requestXXXX()` 関数（例: `requestAccel()`）でリクエストを送信し、`update()` 関数でCANバスの受信バッファを確認し、データがあれば更新します。連続してデータを取得したい場合は、`loop()` 内でリクエストと更新を繰り返す必要があります。
    複数の種類のデータ（加速度、角速度など）を効率的にまとめて取得したい場合に適しています。

    ```cpp
    void loop() {
      // IMUに複数のデータ送信を要求
      imu.requestAccel();
      imu.requestGyro(); // 他のリクエストも並べることができる

      // CANバスから受信したメッセージを全て処理し、内部データを更新
      imu.update();

      // 更新されたデータをgetterで取得する
      float ax = imu.getAccelX();
      float gx = imu.getGyroX();
      float roll = imu.getRoll();

      delay(1);
    }
    ```

-   **`readXXXX()` 関数を使用する方法 (ブロッキング)**

    `readXXXX()` 関数（例: `readAccel()`）を使用すると、IMUへのデータリクエストと、その応答を受信して内部データを更新する処理を一度に行うことができます。この関数は、IMUが応答するまで処理を待機するブロッキング関数です。
    引数としてタイムアウト時間（ミリ秒）を指定できますが、デフォルト値が設定されているため省略も可能です。(30ms)
    特定のデータを1つだけ取得したい場合にはシンプルですが、タイムアウトの待ち時間により関数の実行時間が保証されません。

    ```cpp
    void loop() {
      // 加速度データのリクエストと受信を行う（ブロッキング）
      if (imu.readAccel()) {
        float ax = imu.getAccelX();
        Serial.println(ax);
      } else {
        Serial.println("Failed to read accelerometer data.");
      }
      delay(10);
    }
    ```

#### 2. アクティブモード (Active Mode)

一度設定すると、IMUが指定された種類のデータを一定間隔で自動的に送信し続けるモードです。`setup()`で一度設定すれば、`loop()`内では`update()`を呼び出すだけでデータを更新できます。（永続化もできます）

-   **送信間隔:** `setActiveModeDelay()`かGUIツールで設定可能です。（ただし、FW 1.0.2.4では1msから変動しませんでした。）
-   **送信データ:** 送信するデータの種類（加速度、角速度、オイラー角、四元数）は、現在のファームウェア(1.0.2.4)では公式のGUIツールでのみ設定可能です。

```cpp
void setup() {
  // ... 初期設定 ...

  // アクティブモードに設定
  imu.changeToActive();
}

void loop() {
  // IMUから自動的に送られてくるデータを処理し、値を更新
  imu.update();

  // 更新されたデータをgetterで取得
  float ax = imu.getAccelX();

  delay(1);
}
```

### キャリブレーション

GUIツールで可能なキャリブレーションは、本ライブラリでCAN通信からも実行可能です。

  `accelCalibration()`は加计六面校准（六面キャリブレーション）を実行します。データシートに書かれた順で各面を下にしてIMUを静止させます。ケーブルを繋いで電源供給を行いながらになるため、ケーブルを挿している面を下に向けるのが難しいです。治具が必要になります。

  `gyroCalibration()`,`magCalibration()`に関しては情報が足りていません。

## 参考資料

DAMIAO IMUに関する公式情報や技術資料、GUIツールは、以下のリンクを参照してください。

-   **DAMIAO IMU Giteeリポジトリ**: [https://gitee.com/kit-miao/dm-imu](https://gitee.com/kit-miao/dm-imu)
