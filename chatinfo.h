#ifndef CHATINFO_H
#define CHATINFO_H
#include <vector>
#include <string>
using namespace std;
typedef struct member {
	string name;
	string ipAddress;
	time_t recent_ping;
} member;
#endif
