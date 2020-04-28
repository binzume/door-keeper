# DoorKeeper

ドアをコントロールするサーバ＋クライアント．そのまま動く感じのものじゃないです．

- [Door Keeper](DoorKeeper): サーボでドアのサムターンを回したりするやつ
- [Aiphone Controller](AiphoneCtrl): インターホン(アイホン)を操作するやつ
- [server](server): サーバです

メモ：

- ハードウェアはESP-WROOM-02ベースです(一部AVR)
- UDP hole punchingでNAT越しに通信します
- スマホのホーム画面に登録しとくと良いです(mobile-web-app-capable)

# License

The MIT License
