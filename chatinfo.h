#ifndef CHATINFO_H
#define CHATINFO_H
#include <vector>
using namespace std;
typedef struct member {
	char * name;
	char *ipAddress;
} member;

typedef struct group {
		
	char * groupName;
	vector <member> members;

} group;
#endif
