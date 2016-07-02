//
// フォント利用ライブラリ クラス実装定義 espfonts.cpp
// 作成 2016/05/13 by Tamakichi
// 修正 2016/05/15 by Tamakichi 半角カナ全角変換テーブル、フォント種別テーブルをフラッシュメモリ配置
// 修正 2016/05/16 by Tamakichi インスタンスをグローバル変数化、不具合対応
// 修正 2016/05/17 by Tamakichi fontfile_read()をブロック読み込みに修正
// 修正 2016/05/19 by たま吉さん, グラフィック液晶用フォントモードの追加(setLCDMode()関数追加)
// 修正 2016/06/26 by たま吉さん, ESP8266対応(ARDUINO_ARCH_AVRの判定追加),read_code()の不具合対応
//

#define MYDEBUG 0 
#define USE_CON 0
 
#include "espfonts.h"

#define SD_CS_PIN 10              // SDカード CSピン
#define FONTFILE   "/FONT.BIN"     // フォントファイル名
#define FONT_LFILE "FONTLCD.BIN"  // グラフィック液晶用フォントファイル名

#define RCDSIZ     11

// フォント種別テーブル

static const FontInfo _finfo[EXFONTNUM] = {
  { 0x000000, 0x00017E,  191,  8,  4,  8 }, // 0:u_4x8a.hex
  { 0x000776, 0x000976,  256, 10,  5, 10 }, // 1:u_5x10a.hex
  { 0x001376, 0x001576,  256, 12,  6, 12 }, // 2:u_6x12a.hex
  { 0x002176, 0x002330,  221, 14,  7, 14 }, // 3:u_7x14a.hex
  { 0x002F46, 0x003100,  221, 16,  8, 16 }, // 4:u_8x16a.hex
  { 0x003ED0, 0x00404C,  190, 40, 10, 20 }, // 5:u_10x20a.hex
  { 0x005DFC, 0x005FB6,  221, 48, 12, 24 }, // 6:u_12x24a.hex
  { 0x008926, 0x00BEE4, 6879,  8,  8,  8 }, // 7:u_8x8.hex
  { 0x0195DC, 0x01CB96, 6877, 20, 10, 10 }, // 8:u_10x10.hex
  { 0x03E4DA, 0x041A98, 6879, 24, 12, 12 }, // 9:u_12x12.hex
  { 0x069F80, 0x06D53E, 6879, 28, 14, 14 }, // 10:u_14x14.hex
  { 0x09C5A2, 0x09FB60, 6879, 32, 16, 16 }, // 11:u_16x16.hex
  { 0x0D5740, 0x0D8CFE, 6879, 60, 20, 20 }, // 12:u_20x20.hex
  { 0x13D942, 0x140EFC, 6877, 72, 24, 24 }, // 13:u_24x24.hex
};

// 半角カナ全角変換テーブル
//static PROGMEM const uint8_t _hkremap [] = {
static const uint8_t _hkremap [] = {
   0x02,0x0C,0x0D,0x01,0xFB,0xF2,0xA1,0xA3,0xA5,0xA7,0xA9,0xE3,0xE5,0xE7,0xC3,0xFD,
   0xA2,0xA4,0xA6,0xA8,0xAA,0xAB,0xAD,0xAF,0xB1,0xB3,0xB5,0xB7,0xB9,0xBB,0xBD,0xBF,
   0xC1,0xC4,0xC6,0xC8,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD2,0xD5,0xD8,0xDB,0xDE,0xDF,
   0xE0,0xE1,0xE2,0xE4,0xE6,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEF,0xF3,0x9B,0x9C
};

// 初期化
bool espfonts::init() {
    return SPIFFS.begin();    
}

// グラフィック液晶モードの設定
void espfonts::setLCDMode(bool flg) {
	_lcdmode = flg;
}


// 利用フォント種類の設定 fno : フォント種別番号 (0-13)
void espfonts::setFontNo(uint8_t fno) {
  _fontNo = fno;
}

// 現在の利用フォント種類の取得
uint8_t espfonts::getFontNo() {
  return _fontNo;
}

//利用サイズの設定
void espfonts::setFontSizeAsIndex(uint8_t sz) {
  _fontSize = sz;
  _fontNo = sz+FULL_OFST;
}

// 現在の利用フォントサイズの取得
uint8_t espfonts::getFontSizeIndex() {
  return _fontSize; 
}
// 利用サイズの設定
void espfonts::setFontSize(uint8_t sz) {
  if (sz < 10) 
    setFontSizeAsIndex(EXFONT8);   
  else if (sz < 12) 
    setFontSizeAsIndex(EXFONT10);   
  else if (sz < 14)
    setFontSizeAsIndex(EXFONT12);   
  else if (sz < 16)
    setFontSizeAsIndex(EXFONT14);   
  else if (sz < 20)
    setFontSizeAsIndex(EXFONT16);   
  else if (sz < 24)
    setFontSizeAsIndex(EXFONT20);   
  else if (sz >= 24)
    setFontSizeAsIndex(EXFONT24); 
}

// 現在利用フォントサイズの取得                             
uint8_t espfonts::getFontSize() {
  return getHeight(); 
}

// 現在利用フォントの幅の取得
uint8_t espfonts::getWidth() {
  return _finfo[_fontNo].w;
}
  
// 現在利用フォントの高さの取得
uint8_t espfonts::getHeight() {
  return _finfo[_fontNo].h;
}
  
// 現在利用フォントのデータサイズの取得
uint8_t espfonts::getLength() { 
  return _finfo[_fontNo].b_num;
}

// 半角カナコード判定
uint8_t espfonts::isHkana(uint16_t ucode) {
  if (ucode >=0xFF61 && ucode <= 0xFF9F)
     return 1;
  else 
    return 0;  
}

// 半角カナ全角変換
// JISX0208 -> UTF16の不整合対応
uint16_t espfonts::hkana2kana(uint16_t ucode) {
  if (isHkana(ucode))
    return _hkremap[ucode-0xFF61] + 0x3000; 
  return ucode;
}

uint16_t espfonts::hkana2uhkana(uint16_t ucode) {
  if (isHkana(ucode))
     return ucode - 0xFF61 + 0x0A1;
  return ucode;      
}

//
// フォントファイルのオープン
//
bool espfonts::open() {
  if (_lcdmode)
  fontFile = SPIFFS.open(FONT_LFILE, "r");
  else	
  fontFile = SPIFFS.open(FONTFILE, "r");
	
  if (!fontFile) {
#if MYDEBUG == 1 && USE_CON == 1    
    Serial.print(F("cant open:"));
    Serial.println(F(FONTFILE));
#endif
    return false;
  }
  return true;  
}

//
// ファイルのクローズ
//
void espfonts::close(void) {
  fontFile.close(); 
}

//
// フォントファイルからのデータ取得
// pos(in) 読み込み位置
// dt(out) データ格納領域
// sz(in)  データ取得サイズ
//
bool espfonts::fontfile_read(uint32_t pos, uint8_t* dt, uint8_t sz) {
  if (!fontFile) {
    return false;  
  } else {
    if ( !fontFile.seek(pos,SeekSet) )   
      return false;
  	if (fontFile.read(dt, sz) != sz)
  		return false;	

  }
  return true;  
}

// フォントコード取得
// ファイル上検索テーブルのフォントコードを取得する
// pos(in) フォントコード取得位置
// 戻り値 該当コード or 0xFFFF (該当なし)
//
uint16_t espfonts::read_code(uint16_t pos) {
  uint8_t rcv[2];
  uint32_t addr = _finfo[_fontNo].idx_addr+pos+pos;
  if (!fontfile_read(addr, rcv, 2))  
    return 0xFFFF;
  return  (rcv[0]<<8)+rcv[1]; 
}

// フォントコード検索
// (コードでROM上のテーブルを参照し、フォントコードを取得する)
// ucode(in) UTF-16 コード
// 戻り値    該当フォントがある場合 フォントコード(0-FTABLESIZE)
//           該当フォントが無い場合 -1

int16_t espfonts::findcode(uint16_t  ucode) {
   uint16_t  t_p = 0;                        //　検索範囲上限
   uint16_t  e_p = _finfo[_fontNo].f_num -1; //  検索範囲下限
   uint16_t  pos;
   uint16_t  d = 0;
   int8_t flg_stop = -1;
 
 while(true) {
    pos = t_p + ((e_p - t_p+1)>>1);
    d = read_code (pos);
    if (d==0xFFFF)
      return -1;  
 	
   if (d == ucode) {
     // 等しい
     flg_stop = 1;
     break;
   } else if (ucode > d) {
     // 大きい
     t_p = pos + 1;
     if (t_p > e_p) 
       break;
   } else {
     // 小さい
    e_p = pos -1;
    if (e_p < t_p) 
      break;
   }
 } 

 if (!flg_stop)
    return -1;   
 return pos;   
}

//
// UTF16に対応するフォントデータを取得する
//   data(out): フォントデータ格納アドレス
//   utf16(in): UTF16コード
//   戻り値: true 正常終了１, false 異常終了
//
boolean espfonts::getFontData(byte* fontdata, uint16_t utf16) {
  boolean flgZenkaku = true;
  
  // 文字コードの変更(＼￠￡￢)
  switch(utf16) {
    case 0xFF3C: utf16 = 0x5C;  break;
    case 0xFFE0: utf16 = 0xA2;  break;
    case 0xFFE1: utf16 = 0xA3;  break;
    case 0xFFE2: utf16 = 0xAC;  break;
  }
  
 // 文字コードから全角、半角を判定する
 if (utf16 < 0x3000) {
     switch (utf16) {
       case 0x5C:
       case 0xA2:
       case 0xA3:
       case 0xA7:
       case 0xA8:
       case 0xAC:
       case 0xB0:
       case 0xB1:
       case 0xB4:
       case 0xB6:
       case 0xD7:
       case 0xF7:
         flgZenkaku = true;
         break;
       default:
         flgZenkaku = false;
     } 
   } else 
    
  // 半角カナは全角カナに置き換える
  if (isHkana(utf16)) {
    utf16 = hkana2kana(utf16);
  }
  
  //フォント種別の設定
  if (flgZenkaku) 
    setFontNo(getFontSizeIndex()+FULL_OFST);  // 全角フォント指定
  else
    setFontNo(getFontSizeIndex());            // 半角フォント指定
    
  _code = utf16;
  return getFontDataByUTF16(fontdata, utf16);
}

// 指定したUTF8文字列の先頭のフォントデータの取得
//   data(out): フォントデータ格納アドレス
//   utf8(in) : UTF8文字列
//   戻り値   : 次の文字列位置、取得失敗の場合NULLを返す
//
char* espfonts::getFontData(byte* fontdata,char *pUTF8) {
  uint16_t utf16;
  uint8_t  n;
  if (pUTF8 == NULL)
    return NULL;
  if (*pUTF8 == 0) 
    return NULL;   
  n = charUFT8toUTF16(&utf16, pUTF8);
  if (n == 0)
    return NULL;  
  if (false == getFontData(fontdata, utf16) ) 
    return NULL;
  return (pUTF8+n);
}

//
// UTF16に対応するフォントデータを取得する
//   data(out): フォントデータ格納アドレス
//   utf16(in): UTF16コード
//   戻り値: true 正常終了１, false 異常終了
//
boolean espfonts::getFontDataByUTF16(byte* fontdata, uint16_t utf16) {  
  uint32_t code;
  uint32_t addr;
  uint8_t bnum;
  byte n;
 
  code = findcode(utf16);
  if ( 0 > code)  
    return false;       // 該当するフォントが存在しない
    
  bnum = _finfo[_fontNo].b_num;
  addr = _finfo[_fontNo].dat_addr + code * (uint32_t)bnum;

  return fontfile_read(addr, fontdata, bnum );
}

//
// フォントデータ1行のバイト数の取得
// 戻り値： バイト数
uint8_t espfonts::getRowLength() {
    return ( _finfo[_fontNo].w + 7 ) >>3;
}

//
// UTF8文字(1～3バイト)をUTF16に変換する
// pUTF16(out): UTF16文字列格納アドレス
// pUTF8(in):   UTF8文字列格納アドレス
// 戻り値: 変換処理したUTF8文字バイト数、0の場合は変換失敗

uint8_t espfonts::charUFT8toUTF16(uint16_t *pUTF16, char *pUTF8) { 
  uint8_t  bytes[3]; 
  uint16_t unicode16; 
 
  bytes[0] = *pUTF8++; 
  if( bytes[0] < 0x80 ) { 
   *pUTF16 = bytes[0]; 
   return(1); 
 } 
  bytes[1] = *pUTF8++; 
  if( bytes[0] >= 0xC0 && bytes[0] < 0xE0 )  { 
     unicode16 = 0x1f&bytes[0]; 
     *pUTF16 = (unicode16<<6)+(0x3f&bytes[1]); 
     return(2); 
 } 
 
  bytes[2] = *pUTF8++; 
  if( bytes[0] >= 0xE0 && bytes[0] < 0xF0 ) { 
    unicode16 = 0x0f&bytes[0]; 
    unicode16 = (unicode16<<6)+(0x3f&bytes[1]); 
    *pUTF16 = (unicode16<<6)+(0x3f&bytes[2]); 
    return(3); 
  } else 
  return(0); 
} 

// UTF8文字列をUTF16文字列に変換する
// pUTF16(out): UFT16文字列
// pUTF8(in):   UTF8文字列
// 戻り値: UFT16文字長さ (変換失敗時は-1を返す)
//
uint8_t espfonts::Utf8ToUtf16(uint16_t* pUTF16, char *pUTF8) {
  int len = 0;
  int n;
  uint16_t wstr;

  while (*pUTF8) {
    n = charUFT8toUTF16(pUTF16,pUTF8);
    if (!n) 
      return -1;
    
    pUTF8 += n;
    len++;
    pUTF16++;
  }
  return len; 
}

espfonts ESPfonts;

