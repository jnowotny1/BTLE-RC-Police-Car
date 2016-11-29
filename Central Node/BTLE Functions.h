// BTLE Functions Header File


void Get_ServerServiceList(char *bufPt, unsigned short max);

void Get_ClientServiceList(char *bufPt, unsigned short max);

void Set_SWAKE(char *bufPt);

void Clear_SWAKE(char *bufPt);

void Set_MLDP(char *bufPt);

void Clear_MLDP(char *bufPt);

void Config_CentralRole(char *bufPt, unsigned short max);
	
void Config_PeripheralRole(char *bufPt, unsigned short max);

void Wait_Connection(char *bufPt);

void Central_Scan(char *bufPt, unsigned short max);

void Request_Connection(void);


