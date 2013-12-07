#ifndef CHATINFO_H
#define CHATINFO_H
#include <vector>
#include <string>
using namespace std;
typedef struct member {
	string name;
	char *ipAddress;
} member;

typedef struct group {
		
	string groupName;
	vector <member> members;

} group;
#endif
