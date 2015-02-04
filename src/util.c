#include "util.h"

unsigned long get32bit(const unsigned char* buf, const int start) {
	unsigned long result = (buf[start] << 24) | (buf[start + 1] << 16) | (buf[start + 2] << 8) | buf[start + 3];
	return result;
}
unsigned short get16bit(const unsigned char* buf, const int start) {
	unsigned short result = (buf[start] << 8) | buf[start + 1];
	return result;
}
