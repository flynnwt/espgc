#ifndef IR_H
#define IR_H

#include "GCIT.h"

class CompressedIR {
  static const unsigned int librarySize = 26;
  unsigned int library[librarySize][2];
  unsigned numPairs = 0;
  unsigned int p;

  unsigned int getInt(String s);
  bool isVar(String s);
  unsigned int codePos(char c);
  char codeName(unsigned int n);
  char checkPair(unsigned int on, unsigned int off);
  char checkPairFudged(unsigned int on, unsigned int off, unsigned int fudge);
  char addPair(unsigned int on, unsigned int off);
  char addPairUnique(unsigned int on, unsigned int off);
  unsigned int getPair0(char c);
  unsigned int getPair1(char c);

public:
  unsigned int fudge = 0;
  String decode(String s);
  String encode(String s);
  String encode(String s, unsigned int fudge);
};

#endif