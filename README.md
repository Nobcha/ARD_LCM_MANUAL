# ARD_LCM_MANUAL
The Franclin oscillator LC meter with DPDT-SW calibration 
When assembling an ATU-100 kit or similar, you'll want to know the inductance of air-core coils or toroidal coils. This is necessary to verify that the coils are wound properly.
Many commercially available L meters have a measurement limit of a few μH, and are unable to measure the inductance of low-capacity coils of several tens of nH (around 0.1 μH).
In contrast, this Franklin oscillator LC meter can measure inductances as low as 0.1 μH.

Previous LC meters used a relay for calibration at the start of use, but we've replaced this with a readily available 6P double-pole, double-throw relay, making calibration manual.

There were errors in the circuit diagram and board pattern originally uploaded. Please refer to the corrected circuit diagram.
1. The signal layout of the i2c-OLED 4P header is incorrect.
2. The silk screen for R6 and R7 is incorrect.
3. The signal layout of the ICSP 6P header is incorrect.
Please see LCM V7 Mistake prevention list.jpg for countermeasures.

Although this is a sketch, 95% of the footage was taken using flash, so please be careful when editing or revising.

For the power supply, you will need to devise a TP4066 charging module board, LIPO battery, power switch, etc. Please use the board pattern to modify and wire it.
Please refer to the assembly and operation manual for details.
(continue)

ATU-100キットなどを組み立てるとき、空芯コイルとかトロイダルコイルのインダクタンスが知りたくなります。ちゃんと巻けているのかどうかを確かめるのに必要です。
市販Lメータでは測定下限が数μHぐらいのものが多くて、数十ｎH（0.1μH程度）の低用量コイルのインダクタンスは測れません。
その点このフランクリン発振方式のLCメータは0.1μH程度から測ることも可能です。

従来のLCメータでは使用開始時のキャリブレーションにリレーを用いていましたが、入手容易な６P双極双投に置き換え、キャリブレーションは手動にしました。

最初にアップロードした回路図、基板パターンに誤りがありました。修正した回路図を参照願います。
1.i2c-OLEDの4Pヘッダの信号配置が間違っています。
2.R6,R7のシルクが間違っています。
3.ICSPの6Pヘッダの信号配置が間違っています。
対策についてLCM V7 Mistake prevention list.jpgに書きましたので確認願います。

スケッチですが、フラッシュ使用量が95％となっておりますので、修正・編集の際は注意願います。

電源ですが、TP4066充電モジュール基板、LIPO電池、電源スイッチなどを工夫数必要があります。基板パターンを利用し、改造、配線してください。
細かい点については組み立て、操作説明書を参照願います。
（続く）
