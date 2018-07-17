#include "h8-3069-iodef.h"
#include "h8-3069-int.h"
#include "h8int.h"
#include "loader.h"
#include "timer.h"
#include "lcd.h"
#include "ad.h"

/* $B%?%$%^3d$j9~$_$N;~4V4V3V(B[$B&L(Bs] */
#define TIMER0 500

/* $B3d$j9~$_=hM}$G3F=hM}$r9T$&IQEY$r7h$a$kDj?t(B */
#define DISPTIME 100
#define ADTIME  2
#define PWMTIME 1
#define CONTROLTIME 10

/* PWM$B@)8f4XO"(B */
/* $B@)8f<~4|$r7h$a$kDj?t(B */
#define MAXPWMCOUNT 10

/* LCD$BI=<(4XO"(B */
/* 1$BCJ$KI=<($G$-$kJ8;z?t(B */
#define LCDDISPSIZE 8



/* A/D$BJQ494XO"(B */
/* A/D$BJQ49$N%A%c%M%k?t$H%P%C%U%!%5%$%:(B */
#define ADCHNUM   4
#define ADBUFSIZE 8
/* $BJ?6Q2=$9$k$H$-$N%G!<%?8D?t(B */
#define ADAVRNUM 4
/* $B%A%c%M%k;XDj%(%i!<;~$KJV$9CM(B */
#define ADCHNONE -1

#define MOTOR_FRONT 1
#define MOTOR_BACK 2
#define MOTOR_BRAKE 3
#define MOTOR_STOP 4


volatile int motor_state0;
volatile int motor_state1;

volatile int pwm_count;

/* $B3d$j9~$_=hM}$KI,MW$JJQ?t$OBg0hJQ?t$K$H$k(B */
volatile int disp_time, key_time, ad_time, pwm_time, control_time;

/* LED$B4X78(B */
unsigned char rightval, leftval;


/* A/D$BJQ494X78(B */
volatile unsigned char adbuf[ADCHNUM][ADBUFSIZE];
volatile int adbufdp;

/* LCD$B4X78(B */
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

  /* $B3d$j9~$_$G;HMQ$9$kBg0hJQ?t$N=i4|2=(B */
  pwm_time = pwm_count = 0;     /* PWM$B@)8f4XO"(B */
  disp_time = 0; disp_flag = 1; /* $BI=<(4XO"(B */
  ad_time = 0;                  /* A/D$BJQ494XO"(B */
  control_time = 0;             /* $B@)8f4XO"(B */
  
  /* $B$3$3$^$G(B */
  adbufdp = 0;         /* A/D$BJQ49%G!<%?%P%C%U%!%]%$%s%?$N=i4|2=(B */
  lcd_init();          /* LCD$BI=<(4o$N=i4|2=(B */
  ad_init();           /* A/D$B$N=i4|2=(B */
  timer_init();        /* $B%?%$%^$N=i4|2=(B */
  timer_set(0,TIMER0); /* $B%?%$%^(B0$B$N;~4V4V3V$r%;%C%H(B */
  timer_start(0);      /* $B%?%$%^(B0$B%9%?!<%H(B */
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

  /* $B$3$3$K(BPWM$B=hM}$KJ,4t$9$k$?$a$N=hM}$r=q$/(B */

  pwm_time++;
  if(pwm_time >= PWMTIME){
    pwm_proc();
    pwm_time = 0;
  }

  /* $B$3$3$K@)8f=hM}$KJ,4t$9$k$?$a$N=hM}$r=q$/(B */

  timer_intflag_reset(0); /* $B3d$j9~$_%U%i%0$r%/%j%"(B */
  ENINT();                /* CPU$B$r3d$j9~$_5v2D>uBV$K(B */
}

#pragma interrupt
void int_adi(void)
/* A/D$BJQ49=*N;$N3d$j9~$_%O%s%I%i(B                               */
/* $B4X?t$NL>A0$O%j%s%+%9%/%j%W%H$G8GDj$7$F$$$k(B                   */
/* $B4X?t$ND>A0$K3d$j9~$_%O%s%I%i;XDj$N(B #pragma interrupt $B$,I,MW(B  */
{
  ad_stop();    /* A/D$BJQ49$NDd;_$HJQ49=*N;%U%i%0$N%/%j%"(B */

  /* $B$3$3$G%P%C%U%!%]%$%s%?$N99?7$r9T$&(B */
  /* $B!!C"$7!"%P%C%U%!$N6-3&$KCm0U$7$F99?7$9$k$3$H(B */
  adbufdp++;
  if(adbufdp==ADBUFSIZE) adbufdp=0;

  /* $B$3$3$G%P%C%U%!$K(BA/D$B$N3F%A%c%M%k$NJQ49%G!<%?$rF~$l$k(B */
  /* $B%9%-%c%s%0%k!<%W(B 0 $B$r;XDj$7$?>l9g$O(B */
  /*   A/D ch0$B!A(B3 ($B?.9f@~$G$O(BAN0$B!A(B3)$B$NCM$,(B ADDRAH$B!A(BADDRDH $B$K3JG<$5$l$k(B */
  /* $B%9%-%c%s%0%k!<%W(B 1 $B$r;XDj$7$?>l9g$O(B */
  /*   A/D ch4$B!A(B7 ($B?.9f@~$G$O(BAN4$B!A(B7)$B$NCM$,(B ADDRAH$B!A(BADDRDH $B$K3JG<$5$l$k(B */
  adbuf[1][adbufdp]=ADDRBH;
  adbuf[2][adbufdp]=ADDRCH;
  
  ENINT();      /* $B3d$j9~$_$N5v2D(B */
}

int ad_read(int ch)
/* A/D$B%A%c%M%kHV9f$r0z?t$GM?$($k$H(B, $B;XDj%A%c%M%k$NJ?6Q2=$7$?CM$rJV$94X?t(B */
/* $B%A%c%M%kHV9f$O!$(B0$B!A(BADCHNUM $B$NHO0O(B $B!!!!!!!!!!!!!!!!!!!!!!(B             */
/* $BLa$jCM$O(B, $B;XDj%A%c%M%k$NJ?6Q2=$7$?CM(B ($B%A%c%M%k;XDj%(%i!<;~$O(BADCHNONE) */
{
  int i=0,ad=0,bufpo=0;

  if ((ch > ADCHNUM) || (ch < 0)) ad = ADCHNONE; /* $B%A%c%M%kHO0O$N%A%'%C%/(B */
  else {

    /* $B$3$3$G;XDj%A%c%M%k$N%G!<%?$r%P%C%U%!$+$i(BADAVRNUM$B8D<h$j=P$7$FJ?6Q$9$k(B */
    /* $B%G!<%?$r<h$j=P$9$H$-$K!"%P%C%U%!$N6-3&$KCm0U$9$k$3$H(B */
    /* $BJ?6Q$7$?CM$,La$jCM$H$J$k(B */
    ad = 0;
    bufpo = adbufdp + ADBUFSIZE;

    for(i=0; i<ADAVRNUM; i++){
      ad = ad + adbuf[ch][(bufpo-i) % ADBUFSIZE];
    }


    ad=ad/ADAVRNUM;

  }
  return ad; /* $B%G!<%?$NJ?6QCM$rJV$9(B */
}


void pwm_proc(void)
  /* PWM$B@)8f$r9T$&4X?t(B                                        */
  /* $B$3$N4X?t$O%?%$%^3d$j9~$_(B0$B$N3d$j9~$_%O%s%I%i$+$i8F$S=P$5$l$k(B */
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
     /* $B@)8f$r9T$&4X?t(B                                           */
     /* $B$3$N4X?t$O%?%$%^3d$j9~$_(B0$B$N3d$j9~$_%O%s%I%i$+$i8F$S=P$5$l$k(B */
{
  /* $B$3$3$K@)8f=hM}$r=q$/(B */
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


