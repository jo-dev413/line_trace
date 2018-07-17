#include "h8-3069-iodef.h"
#include "h8-3069-int.h"
#include "h8int.h"
#include "loader.h"
#include "timer.h"
#include "lcd.h"
#include "ad.h"

/* タイマ割り込みの時間間隔[μs] */
#define TIMER0 500

/* 割り込み処理で各処理を行う頻度を決める定数 */
#define DISPTIME 100
#define ADTIME  2
#define PWMTIME 1
#define CONTROLTIME 10

/* PWM制御関連 */
/* 制御周期を決める定数 */
#define MAXPWMCOUNT 10

/* LCD表示関連 */
/* 1段に表示できる文字数 */
#define LCDDISPSIZE 8



/* A/D変換関連 */
/* A/D変換のチャネル数とバッファサイズ */
#define ADCHNUM   4
#define ADBUFSIZE 8
/* 平均化するときのデータ個数 */
#define ADAVRNUM 4
/* チャネル指定エラー時に返す値 */
#define ADCHNONE -1

#define MOTOR_FRONT 1
#define MOTOR_BACK 2
#define MOTOR_BRAKE 3
#define MOTOR_STOP 4


volatile int motor_state0;
volatile int motor_state1;

volatile int pwm_count;

/* 割り込み処理に必要な変数は大域変数にとる */
volatile int disp_time, key_time, ad_time, pwm_time, control_time;

/* LED関係 */
unsigned char rightval, leftval;


/* A/D変換関係 */
volatile unsigned char adbuf[ADCHNUM][ADBUFSIZE];
volatile int adbufdp;

/* LCD関係 */
volatile int disp_flag;
volatile char lcd_str_upper[LCDDISPSIZE+1];
volatile char lcd_str_lower[LCDDISPSIZE+1];



void int_imia0(void);
void pwm_proc(void);
void control_proc(void);
void int_adi(void);
int  ad_read(int ch);

void move_back0(void);
void move_back1(void);

void move_front0(void);
void move_front1(void);

void stop0(void);
void stop1(void);

void brake0(void);
void brake1(void);

int main(void){
  int i;
  char box,box2;

  PBDDR|=0x0F;
  ROMEMU();

  /* 割り込みで使用する大域変数の初期化 */
  pwm_time = pwm_count = 0;     /* PWM制御関連 */
  disp_time = 0; disp_flag = 1; /* 表示関連 */
  ad_time = 0;                  /* A/D変換関連 */
  control_time = 0;             /* 制御関連 */
  
  /* ここまで */
  adbufdp = 0;         /* A/D変換データバッファポインタの初期化 */
  lcd_init();          /* LCD表示器の初期化 */
  ad_init();           /* A/Dの初期化 */
  timer_init();        /* タイマの初期化 */
  timer_set(0,TIMER0); /* タイマ0の時間間隔をセット */
  timer_start(0);      /* タイマ0スタート */
  pwm_count=0;
  rightval = leftval = 0;
  ENINT();

  

  strcpy(lcd_str_upper,"LEF:    ");
  strcpy(lcd_str_lower,"RIG:    ");


  while(1){
    if(leftval==255){
      lcd_str_upper[4]=2+'0';
      lcd_str_upper[5]=5+'0';
      lcd_str_upper[6]=5+'0';
    }

    if(0<=leftval && leftval<10){
      lcd_str_upper[4]='0';
      lcd_str_upper[5]='0';
      lcd_str_upper[6]=leftval+'0';
    }else if(leftval>=10 && leftval<100){
      lcd_str_upper[4]='0';
      lcd_str_upper[5]=leftval/10+'0';
      lcd_str_upper[6]=leftval%10+'0';
    }else{
      lcd_str_upper[6]=leftval%10+'0';
      box=leftval/10;
      lcd_str_upper[4]=box/10+'0';
      lcd_str_upper[5]=box%10+'0';
    }

    if(rightval==255){
      lcd_str_lower[4]=2+'0';
      lcd_str_lower[5]=5+'0';
      lcd_str_lower[6]=5+'0';
    }

    if(0<=rightval && rightval<10){
      lcd_str_lower[4]='0';
      lcd_str_lower[5]='0';
      lcd_str_lower[6]=rightval+'0';
    }else if(rightval>=10 && rightval<100){
      lcd_str_lower[4]='0';
      lcd_str_lower[5]=rightval/10+'0';
      lcd_str_lower[6]=rightval%10+'0';
    }else{
      lcd_str_lower[6]=rightval%10+'0';
      box2=rightval/10;
      lcd_str_lower[4]=box2/10+'0';
      lcd_str_lower[5]=box2%10+'0';
    }

    if(disp_flag==1){

      for (i = 0; i < 8; i++){
        lcd_cursor(i,0);
        lcd_printch(lcd_str_upper[i]);
      }
      for (i = 0; i < 8; i++){
        lcd_cursor(i,1);
        lcd_printch(lcd_str_lower[i]);
      }

    } 
  }
  return 0;
}

#pragma interrupt
void int_imia0(void){
 
  disp_time++;
  if (disp_time >= DISPTIME){
    disp_flag = 1;
    disp_time = 0;
  }

  control_time++;
  if(control_time==CONTROLTIME){
    control_proc();
    control_time=0;
  }

  ad_time++;
  if(ad_time==ADTIME){
    ad_scan(0,1);
    ad_time=0;
  }

  /* ここにPWM処理に分岐するための処理を書く */

  pwm_time++;
  if(pwm_time >= PWMTIME){
    pwm_proc();
    pwm_time = 0;
  }

  /* ここに制御処理に分岐するための処理を書く */

  timer_intflag_reset(0); /* 割り込みフラグをクリア */
  ENINT();                /* CPUを割り込み許可状態に */
}

#pragma interrupt
void int_adi(void)
/* A/D変換終了の割り込みハンドラ                               */
/* 関数の名前はリンカスクリプトで固定している                   */
/* 関数の直前に割り込みハンドラ指定の #pragma interrupt が必要  */
{
  ad_stop();    /* A/D変換の停止と変換終了フラグのクリア */

  /* ここでバッファポインタの更新を行う */
  /* 　但し、バッファの境界に注意して更新すること */
  adbufdp++;
  if(adbufdp==ADBUFSIZE) adbufdp=0;

  /* ここでバッファにA/Dの各チャネルの変換データを入れる */
  /* スキャングループ 0 を指定した場合は */
  /*   A/D ch0〜3 (信号線ではAN0〜3)の値が ADDRAH〜ADDRDH に格納される */
  /* スキャングループ 1 を指定した場合は */
  /*   A/D ch4〜7 (信号線ではAN4〜7)の値が ADDRAH〜ADDRDH に格納される */
  adbuf[1][adbufdp]=ADDRBH;
  adbuf[2][adbufdp]=ADDRCH;
  
  ENINT();      /* 割り込みの許可 */
}

int ad_read(int ch)
/* A/Dチャネル番号を引数で与えると, 指定チャネルの平均化した値を返す関数 */
/* チャネル番号は，0〜ADCHNUM の範囲 　　　　　　　　　　　             */
/* 戻り値は, 指定チャネルの平均化した値 (チャネル指定エラー時はADCHNONE) */
{
  int i=0,ad=0,bufpo=0;

  if ((ch > ADCHNUM) || (ch < 0)) ad = ADCHNONE; /* チャネル範囲のチェック */
  else {

    /* ここで指定チャネルのデータをバッファからADAVRNUM個取り出して平均する */
    /* データを取り出すときに、バッファの境界に注意すること */
    /* 平均した値が戻り値となる */
    ad = 0;
    bufpo = adbufdp + ADBUFSIZE;

    for(i=0; i<ADAVRNUM; i++){
      ad = ad + adbuf[ch][(bufpo-i) % ADBUFSIZE];
    }


    ad=ad/ADAVRNUM;

  }
  return ad; /* データの平均値を返す */
}


void pwm_proc(void)
  /* PWM制御を行う関数                                        */
  /* この関数はタイマ割り込み0の割り込みハンドラから呼び出される */
{
  //init pwm_count
  if(pwm_count == MAXPWMCOUNT){
    pwm_count = 0;
  }

  //move front
  if(leftval < 140 && rightval < 140){
    move_front0();  
    move_front1();
  }

  //move left 
  else if(leftval < rightval){
    stop0();
    move_front1();
  }

  //move right
  else if(leftval > rightval){
    stop1();
    move_front0();
  }
  
  pwm_count++;
}

void control_proc(void)
     /* 制御を行う関数                                           */
     /* この関数はタイマ割り込み0の割り込みハンドラから呼び出される */
{
  /* ここに制御処理を書く */
  leftval=ad_read(1);
  rightval=ad_read(2);
}

void move_front0(void)
{
  if(motor_state0 == MOTOR_BACK || motor_state0 == MOTOR_BRAKE){
    motor_state0 = MOTOR_FRONT;
    stop0();
  }else{
    PBDR = PBDR | 0x01; //0b0000 0001
    PBDR = PBDR & 0xFD; //0b1111 1101
    motor_state0 = MOTOR_FRONT;
  }
}
void move_front1(void)
{
  if(motor_state1 == MOTOR_BACK || motor_state1 == MOTOR_BRAKE){
    motor_state1 = MOTOR_FRONT;
    stop1();
  }else{
    PBDR = PBDR | 0x04; //0b0000 0100
    PBDR = PBDR & 0xF7; //0b1111 0111
    motor_state1 = MOTOR_FRONT;
  }
}

void move_back0(void)
{ 
  if(motor_state0 == MOTOR_FRONT || motor_state0 == MOTOR_BRAKE){
    motor_state0 = MOTOR_BACK;
    stop0();
  }else{
    PBDR = PBDR | 0x02; //0b0000 0010
    PBDR = PBDR & 0xFE; //0b1111 1110
    motor_state0 = MOTOR_BACK;
  }
}

void move_back1(void)
{
  if(motor_state1 == MOTOR_FRONT || motor_state1 == MOTOR_BRAKE){
    motor_state1 = MOTOR_BACK;
    stop1();
  }else{
    PBDR = PBDR | 0x08; //0b0000 1000
    PBDR = PBDR & 0xFB; //0b1111 1011
    motor_state1 = MOTOR_BACK;
  }
}

void stop0(void)
{
  PBDR = PBDR & 0xFC; //0b1111 1100
}

void stop1(void)
{
  PBDR = PBDR & 0xF3; //0b1111 0011
}

void brake0(void)
{
  if(motor_state0 == MOTOR_FRONT || motor_state0 == MOTOR_BACK){
    motor_state0 = MOTOR_BRAKE;
    stop0();
  }else{
    PBDR = PBDR | 0x03; //0b0000 0011
    motor_state0 = MOTOR_BRAKE; 
  }
}

void brake1(void)
{
  if(motor_state1 == MOTOR_FRONT || motor_state1 == MOTOR_BACK){
    motor_state1 = MOTOR_BRAKE;
    stop0();
  }else{
    PBDR = PBDR | 0x0C; //0b0000 1100
    motor_state1 = MOTOR_BRAKE; 
  }
}


