
# メモ

## front:

- 1 通常LED K TP465
- 2 GND
- 3 非常 LED K + 電源LED K
- 4 LED R
- 5 LED L
- 10 ボタンcommon TP456 28ms main:TP455
- 13 vcc 5.0V
- 14 開錠ボタン TP210
- 15 Rボタン
- 16 Lボタン
- 17 非常LED A TP399
- 18 電源LED A TP401
- 22 TP464
- 23 情報LED
- 24 開錠LED TP462

## main:

- 通話ボタン TP209
- 通話LED TP229

## 色々

- ボタン周りはたぶんマトリクス，ボタンcommonに矩形波が出ている．
- 表に出てるボタンの読み出しタイミングはたぶん同じ．
- 通常のボタンの間に，部屋番号上位4ビット，下位4ビット，その他設定のディップスイッチを読んでいる
- Vcc 5.0Vは少量なら電流取り出しても問題なさげ．基板上から取らなくてもコネクタに出てる．
