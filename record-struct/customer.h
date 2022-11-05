#ifndef CUSTOMER_RECORD
#define CUSTOMER_RECORD

struct Customer
{
    int id; 
    char name[25];
    char gender;
    int age;
    
    char login[30]; 
    char password[30];
    
    int account; 
};

#endif