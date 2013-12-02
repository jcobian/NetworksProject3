#include <vector>

typedef struct member {
	char * name;
	char * ipAddress;
} member;

typedef struct group {
		
	char * groupName;
	vector <member> members;

} group;
