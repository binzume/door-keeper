# インターホンをどうにかするやつ

作った経緯は，[アイ〇ンのプロトコルの話](https://qiita.com/binzume/items/54ea81600ba8cfd4a14b)にあります．

基本的にメモやコード片なのでそのままでは使えないです．


## AiphoneCtrl.ino

Wi-Fiでインターホンをコントロールするやつ．回路図ないですがリレー等で操作する感じです．回路作る場合はmemo.mdも参照．

ESP-WROOM-02で動作します．

## avr, avr_tiny13

アイホンのエントランスと室内端末の通信を受信するやつ．(コード上には送信用の処理がありますが，怪しいので使わないほうが良いかも)


AVR内蔵のアナログコンパレータでも動くようにしました．

コード中の USE_COMPARATOR を 1 にしてください． tiny13等の8ピンAVRでも動くので，ほぼそのままNJM2903を置き換えられます．

![回路2](circuit/circuit_avr_only.png)

古いやつ(avr)：

![受信回路](circuit/circuit.png)
