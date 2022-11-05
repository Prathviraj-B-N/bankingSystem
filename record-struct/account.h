#ifndef ACCOUNT_RECORD
#define ACCOUNT_RECORD

struct Account
{
    int accountNumber;     
    int owners[2];         
    bool isRegularAccount; // 1 : reg
    bool active;           
    long int balance;      
};

#endif