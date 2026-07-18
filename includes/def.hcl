// DEF.HCL /27/10/96
#include<math.h>
//#include<complex.h>
//#include<bcd.h>
#include<float.h>
//#include<graphics.h>
#include<stdlib.h>
#include<conio.h>
#include<stdio.h>
#include<ctype.h>
#include<values.h>
#include<string.h>
#include<dos.h>
//#include<time.h>
//#include<process.h>
#include<bios.h>

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long double Ldbl;

#define PIx2 6.28318530717958647692
#define aspx .866025403//1.
#define aspy 1.154700538//.725
#define BCinf while(1)
#define _(x) (*(x))
#define _L(x) ((Ldbl)*(x))
#define LOG(x) log10l(x)
#define LN(x)  logl(x)
#define pw2(x) (x)*(x)
#define pw3(x) (x)*(x)*(x)
#define pw10(x) powl(10.,x)
#define inv(x) (1./(Ldbl)(x))
#define root(x,y) powl(x,1./(Ldbl)(y))
#define cubrt(x) powl(x,0.33333333333333333333)
#define inhI() disable()
#define enI() enable()
#define INTR(x) geninterrupt(x)

#define MAIN void main(void){ ctrlbrk(c_break);
#define END }

char GETCH();
char inkey();
char INKEY();
int c_break(void);

char GETCH()
{
 return(toupper(getch()));
}
char inkey()
{
 char t;
 if(kbhit()) t=getch();
 else t=' ';
 return(t);
}
char INKEY()
{
 return(toupper(inkey()));
}
template<class T>
T iif(int cmp,T a,T b)
{
 if(cmp) return a;
 else return b;
}
#define ABORT 0
int c_break(void)
{
 printf("Control-Break pressed.  Program aborting ...\n");
 return ABORT;
}
#define CTRLBRK ctrlbrk(c_break)