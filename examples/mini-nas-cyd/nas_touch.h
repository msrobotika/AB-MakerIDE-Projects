#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Dedicated software SPI: leaves VSPI free for the microSD.
namespace touchpad {
constexpr int CLK=25, DIN=32, CS=33, IRQ=36, DOUT=39;
float ax=0, bx=0, cx=0, ay=0, by=0, cy=0;
bool calibrated=false;
uint8_t transfer(uint8_t value) {
  uint8_t result=0;
  for (int i=7;i>=0;--i) {
    digitalWrite(DIN,(value>>i)&1); digitalWrite(CLK,HIGH); delayMicroseconds(1);
    result=(result<<1)|digitalRead(DOUT); digitalWrite(CLK,LOW); delayMicroseconds(1);
  }
  return result;
}
uint16_t convert(uint8_t command) {
  transfer(command); uint16_t value=uint16_t(transfer(0))<<8; value|=transfer(0); return (value>>3)&4095;
}
void begin() {
  pinMode(CLK,OUTPUT);pinMode(DIN,OUTPUT);pinMode(CS,OUTPUT);pinMode(DOUT,INPUT);pinMode(IRQ,INPUT);
  digitalWrite(CLK,LOW);digitalWrite(CS,HIGH);
  Preferences p;p.begin("msrnas-touch",true);
  float data[6];
  if (p.getBytesLength("affine")==sizeof(data) && p.getBytes("affine",data,sizeof(data))==sizeof(data)) {
    ax=data[0];bx=data[1];cx=data[2];ay=data[3];by=data[4];cy=data[5];calibrated=true;
  }
  p.end();
}
bool raw(int& x,int& y) {
  if (digitalRead(IRQ)!=LOW) return false;
  digitalWrite(CS,LOW);convert(0xD1);
  int xs[3],ys[3];for(int i=0;i<3;++i){xs[i]=convert(0xD1);ys[i]=convert(0x91);}
  // PD=00 returns the controller to power-down and enables PENIRQ.
  convert(0x90);digitalWrite(CS,HIGH);
  auto median=[](int a,int b,int c){return max(min(a,b),min(max(a,b),c));};
  x=median(xs[0],xs[1],xs[2]);y=median(ys[0],ys[1],ys[2]);
  return x>50&&x<4050&&y>50&&y<4050;
}
bool point(int& x,int& y) {
  int rx,ry;if(!calibrated||!raw(rx,ry))return false;
  x=int(ax*rx+bx*ry+cx);y=int(ay*rx+by*ry+cy);return x>=0&&x<240&&y>=0&&y<320;
}
bool fit(const int* rx,const int* ry,const int* sx,const int* sy) {
  float det=rx[0]*(ry[1]-ry[2])+rx[1]*(ry[2]-ry[0])+rx[2]*(ry[0]-ry[1]);
  if(abs(det)<10000)return false;
  auto solve=[&](const int* s,float& a,float& b,float& c){
    a=(s[0]*(ry[1]-ry[2])+s[1]*(ry[2]-ry[0])+s[2]*(ry[0]-ry[1]))/det;
    b=(s[0]*(rx[2]-rx[1])+s[1]*(rx[0]-rx[2])+s[2]*(rx[1]-rx[0]))/det;
    c=s[0]-a*rx[0]-b*ry[0];
  };
  solve(sx,ax,bx,cx);solve(sy,ay,by,cy);calibrated=true;
  Preferences p;p.begin("msrnas-touch",false);float data[]={ax,bx,cx,ay,by,cy};p.putBytes("affine",data,sizeof(data));p.end();return true;
}
}
