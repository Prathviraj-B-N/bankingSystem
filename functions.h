#ifndef COMMON_FUNCTIONS
#define COMMON_FUNCTIONS

#include <stdio.h>   
#include <unistd.h>    
#include <string.h>    
#include <stdbool.h>   
#include <sys/types.h> 
#include <sys/stat.h>  
#include <fcntl.h>    
#include <stdlib.h>    
#include <errno.h>     
#include <sys/ipc.h>
#include <sys/sem.h>

#include "./record-struct/account.h"
#include "./record-struct/customer.h"


#define ACCOUNT_FILE "./records/account.bank"
#define CUSTOMER_FILE "./records/customer.bank"
#define ADMIN_LOGIN_ID "a"
#define ADMIN_PASSWORD "1"

bool login_handler(bool isAdmin, int connFD, struct Customer *ptrToCustomer);
bool get_account_details(int connFD, struct Account *customerAccount);
bool get_customer_details(int connFD, int customerID);
bool get_transaction_details(int connFD, int accountNumber);
bool admin(int connFD);
bool add_account(int connFD);
int add_customer(int connFD, bool isPrimary, int newAccountNumber);
bool delete_account(int connFD);
bool modify_customer_info(int connFD);
bool customer(int connFD);
bool deposit(int connFD);
bool withdraw(int connFD);
bool get_balance(int connFD);
bool change_password(int connFD);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);
bool exit1(){ return true ;}

struct Customer loggedInCustomer;
int semIdentifier;

bool login_handler(bool isAdmin, int connFD, struct Customer *cust){
    ssize_t readBytes, writeBytes;            
    char readBuffer[1000], writeBuffer[1000]; 
    char tempBuffer[1000];
    struct Customer customer;

    int ID;

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));

    if (isAdmin)
        strcpy(writeBuffer, "Welcome admin!");
    else
        strcpy(writeBuffer, "Welcome customer!");

    strcat(writeBuffer, "\n");
    strcat(writeBuffer, "Login ID : ");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("login!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }

    bool userFound = false;

    if (isAdmin)
    {
        if (strcmp(readBuffer, ADMIN_LOGIN_ID) == 0)
            userFound = true;
    }
    else
    {
        bzero(tempBuffer, sizeof(tempBuffer));
        strcpy(tempBuffer, readBuffer);
        strtok(tempBuffer, "-");
        ID = atoi(strtok(NULL, "-"));

        int customerFileFD = open(CUSTOMER_FILE, O_RDONLY);
        if (customerFileFD == -1)
        {
            perror("Error opening customer file in read mode!");
            return false;
        }

        off_t offset = lseek(customerFileFD, ID * sizeof(struct Customer), SEEK_SET);
        if (offset >= 0)
        {
            struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Customer), sizeof(struct Customer), getpid()};

            int lockingStatus = fcntl(customerFileFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on customer record!");
                return false;
            }

            readBytes = read(customerFileFD, &customer, sizeof(struct Customer));
            if (readBytes == -1)
            {
                ;
                perror("Error reading customer record from file!");
            }

            lock.l_type = F_UNLCK;
            fcntl(customerFileFD, F_SETLK, &lock);

            if (strcmp(customer.login, readBuffer) == 0)
                userFound = true;

            close(customerFileFD);
        }
        else
        {
            writeBytes = write(connFD, "login ID not found$", strlen("login ID not found$"));
        }
    }

    if (userFound)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, "Password : ", strlen("Password : "));
        if (writeBytes == -1)
        {
            perror("Error writing password message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading password from the client!");
            return false;
        }

        if (isAdmin)
        {
            if (strcmp(readBuffer, ADMIN_PASSWORD) == 0)
                return true;
            else{
                perror("incorrect password from the client!");
            return false;
            }
        }
        else
        {
            if (strcmp(readBuffer, customer.password) == 0)
            {
                *cust = customer;
                return true;
            }
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, "Invalid password$", strlen("Invalid password$"));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, "Invalid password$", strlen("Invalid password$"));
    }

    return false;
}

bool get_account_details(int connFD, struct Account *customerAccount){
    ssize_t readBytes, writeBytes;            
    char readBuffer[1000], writeBuffer[1000]; 
    char tempBuffer[1000];

    int accountNumber;
    struct Account account;
    int accountFileDescriptor;

    if (customerAccount == NULL)
    {

        writeBytes = write(connFD, "Acc No : ", strlen("Acc No : "));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from client!");
            return false;
        }

        accountNumber = atoi(readBuffer);
    }
    else
        accountNumber = customerAccount->accountNumber;

    accountFileDescriptor = open(ACCOUNT_FILE, O_RDONLY);
    if (accountFileDescriptor == -1)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc not found");
        strcat(writeBuffer, "^");
        perror("Error opening account file in get_account_details!");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror(" writing  message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Account), SEEK_SET);
    if (offset == -1 && errno == EINVAL)
    {
        // Account record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc not found");
        strcat(writeBuffer, "^");
        perror("Error seeking to account record in get_account_details!");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror(" writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror(" seeking to required account record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};

    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on account record!");
        return false;
    }

    readBytes = read(accountFileDescriptor, &account, sizeof(struct Account));
    if (readBytes == -1)
    {
        perror("Error reading account record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    if (customerAccount != NULL)
    {
        *customerAccount = account;
        return true;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "Account Details - \n\tAccount Number : %d\n\tAccount Type : %s\n\tAccount Status : %s", account.accountNumber, (account.isRegularAccount ? "Regular" : "Joint"), (account.active) ? "Active" : "Deactived");
    if (account.active)
    {
        sprintf(tempBuffer, "\n\tAccount Balance:₹ %ld", account.balance);
        strcat(writeBuffer, tempBuffer);
    }

    sprintf(tempBuffer, "\n\tPrimary Owner ID: %d", account.owners[0]);
    strcat(writeBuffer, tempBuffer);
    if (account.owners[1] != -1)
    {
        sprintf(tempBuffer, "\n\tSecondary Owner ID: %d", account.owners[1]);
        strcat(writeBuffer, tempBuffer);
    }

    strcat(writeBuffer, "\n^");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}

bool get_customer_details(int connFD, int customerID){
    ssize_t readBytes, writeBytes;             // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[10000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];

    struct Customer customer;
    int customerFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Account), getpid()};

    if (customerID == -1)
    {
        writeBytes = write(connFD, "Customer ID : ", strlen("Customer ID : "));
        if (writeBytes == -1)
        {
            perror(" writing message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error getting customer ID from client!");
            ;
            return false;
        }

        customerID = atoi(readBuffer);
    }

    customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc dosent exist");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror(" writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    int offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc dosent exist");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror(" writing  message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror(" seeking to required customer record!");
        return false;
    }
    lock.l_start = offset;

    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror(" obtaining read lock on the Customer file!");
        return false;
    }

    readBytes = read(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (readBytes == -1)
    {
        perror("Error reading customer record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "Customer Details - \n\tID : %d\n\tName : %s\n\tGender : %c\n\tAge: %d\n\tAccount Number : %d\n\tLoginID : %s", customer.id, customer.name, customer.gender, customer.age, customer.account, customer.login);

    strcat(writeBuffer, "\n\nYou'll now be redirected to the main menu...^");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing customer info to client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
}

bool admin(int connFD){

    if (login_handler(true, connFD, NULL))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client
        bzero(writeBuffer, sizeof(writeBuffer));
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, "1. Get Customer Details\n2. Get Account Details\n3. Add Account\n4. Delete Account\n5. Modify Customer Information\nPress any other key to logout");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, -1); // -1 to indicate admin
                break;
            case 2:
                get_account_details(connFD, NULL); // NULL for admin cuz we dont want to store any acc details like customer
                break;
            case 3:
                add_account(connFD);
                break;
            case 4:
                delete_account(connFD);
                break;
            case 5:
                modify_customer_info(connFD);
                break;
            default:
                writeBytes = write(connFD, "Logging out...$", strlen("Logging out...$"));
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool add_account(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Account newAccount, prevAccount;

    int accfd = open(ACCOUNT_FILE, O_RDONLY);
    if (accfd == -1 && errno == ENOENT)
    {
        newAccount.accountNumber = 0;
    }
    else if (accfd == -1)
    {
        perror("opening account file");
        return false;
    }
    else
    {
        int offset = lseek(accfd, -sizeof(struct Account), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Account record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
        int lockingStatus = fcntl(accfd, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Account record!");
            return false;
        }

        readBytes = read(accfd, &prevAccount, sizeof(struct Account));
        if (readBytes == -1)
        {
            perror("reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accfd, F_SETLK, &lock);

        close(accfd);

        newAccount.accountNumber = prevAccount.accountNumber + 1;
    }
    writeBytes = write(connFD, "1. regular account\n2. joint account", strlen("1. regular account\n2. joint account"));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_ACCOUNT_TYPE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account type response from client!");
        return false;
    }

    newAccount.isRegularAccount = atoi(readBuffer) == 1 ? true : false;

    newAccount.owners[0] = add_customer(connFD, true, newAccount.accountNumber);

    if (newAccount.isRegularAccount)
        newAccount.owners[1] = -1;
    else
        newAccount.owners[1] = add_customer(connFD, false, newAccount.accountNumber);

    newAccount.active = true;
    newAccount.balance = 0;

    accfd = open(ACCOUNT_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (accfd == -1)
    {
        perror("creating / opening account file!");
        return false;
    }

    writeBytes = write(accfd, &newAccount, sizeof(struct Account));
    if (writeBytes == -1)
    {
        perror("writing Account record to file!");
        return false;
    }

    close(accfd);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "%s%d", "Acc no :", newAccount.accountNumber);
    strcat(writeBuffer, "\nRedirecting you to the main menu ...^");
    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    readBytes = read(connFD, readBuffer, sizeof(read)); 
    return true;
}

int add_customer(int connFD, bool isPrimary, int newAccountNumber){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Customer newCustomer, previousCustomer;

    int customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1 && errno == ENOENT)
    {
        newCustomer.id = 0;
    }
    else if (customerFileDescriptor == -1)
    {
        perror("opening customer file");
        return -1;
    }
    else
    {
        int offset = lseek(customerFileDescriptor, -sizeof(struct Customer), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Customer record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
        int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Customer record!");
            return false;
        }

        readBytes = read(customerFileDescriptor, &previousCustomer, sizeof(struct Customer));
        if (readBytes == -1)
        {
            perror("reading Customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFileDescriptor, F_SETLK, &lock);

        close(customerFileDescriptor);

        newCustomer.id = previousCustomer.id + 1;
    }

    if (isPrimary)
        sprintf(writeBuffer, "%s%s", "Enter first customer details\n", "name : ");
    else
        sprintf(writeBuffer, "%s%s", "Enter second customer details\n", "name : ");

    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("reading customer name!");
        ;
        return false;
    }

    strcpy(newCustomer.name, readBuffer);

    writeBytes = write(connFD, "Gender(M/F) : ", strlen("Gender(M/F) : "));
    if (writeBytes == -1)
    {
        perror("Error writing message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("reading customer response from client!");
        return false;
    }

    if (readBuffer[0] == 'M' || readBuffer[0] == 'F')
        newCustomer.gender = readBuffer[0];
    else
    {
        writeBytes = write(connFD, "Enter M/F for gender ! ^", strlen("Enter M/F for gender ! ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); 
        return false;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Age : ");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("reading age response from client!");
        return false;
    }

    int customerAge = atoi(readBuffer);
    if (customerAge == 0)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Invalid age!^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); 
        return false;
    }
    newCustomer.age = customerAge;

    newCustomer.account = newAccountNumber;

    strcpy(newCustomer.login, newCustomer.name);
    strcat(newCustomer.login,"-");
    sprintf(writeBuffer, "%d", newCustomer.id);
    strcat(newCustomer.login,writeBuffer);

    char temp[100];
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(temp,newCustomer.name);
    sprintf(writeBuffer, "%d", newCustomer.age);
    strcat(temp,writeBuffer);
    strcpy(newCustomer.password, temp);

    customerFileDescriptor = open(CUSTOMER_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (customerFileDescriptor == -1)
    {
        perror("creating / opening customer file!");
        return false;
    }
    writeBytes = write(customerFileDescriptor, &newCustomer, sizeof(newCustomer));
    if (writeBytes == -1)
    {
        perror("writing Customer record to file!");
        return false;
    }

    close(customerFileDescriptor);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "%s%s\n%s%s", "Loging ID : ", newCustomer.login, "Password : ", newCustomer.password);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("sending cred to the client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); 

    return newCustomer.id;
}

bool delete_account(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Account account;

    writeBytes = write(connFD, "Acc No : ", strlen("Acc No : "));
    if (writeBytes == -1)
    {
        perror("Error writing to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account number response from the client!");
        return false;
    }

    int accountNumber = atoi(readBuffer);

    int accountFileDescriptor = open(ACCOUNT_FILE, O_RDONLY);
    if (accountFileDescriptor == -1)
    {
        // Account record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc not found");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }


    int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Account), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Acc not found");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("seeking to required account record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on Account record!");
        return false;
    }

    readBytes = read(accountFileDescriptor, &account, sizeof(struct Account));
    if (readBytes == -1)
    {
        perror("reading Account record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    close(accountFileDescriptor);

    bzero(writeBuffer, sizeof(writeBuffer));
    
    account.active = false;
    accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
    if (accountFileDescriptor == -1)
    {
        perror("Error opening Account file in write mode!");
        return false;
    }
    offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Account), SEEK_SET);
    if (offset == -1)
    {
        perror("Error seeking to the Account!");
        return false;
    }
    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    int lockingStat = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStat == -1)
    {
        perror("Error obtaining write lock on the Account file!");
        return false;
    }
    writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
    if (writeBytes == -1)
    {
        perror("Error deleting account record!");
        return false;
    }
    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    strcpy(writeBuffer, "Deleted..^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("writing final message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}

bool modify_customer_info(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Customer customer;

    int customerID;

    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, "Customer ID : ", strlen("Customer ID : "));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("reading from client!");
        return false;
    }

    customerID = atoi(readBuffer);

    int customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "No such customer");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
    offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "No such customer");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("seeking customer record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on customer record!");
        return false;
    }

    readBytes = read(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (readBytes == -1)
    {
        perror("reading customer record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);

    close(customerFileDescriptor);

    writeBytes = write(connFD, "Choose...\n1. Name 2. Age 3. Gender 4. cancel", strlen("Choose...\n1. Name 2. Age 3. Gender 4. cancel"));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("read from client!");
        return false;
    }

    int choice = atoi(readBuffer);
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Invalid Input!^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    switch (choice)
    {
    case 1:
        writeBytes = write(connFD, "New name : ", strlen("New name : "));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("getting response from client!");
            return false;
        }
        strcpy(customer.name, readBuffer);
        break;
    case 2:
        writeBytes = write(connFD, "New age : ", strlen("New age : "));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("getting response from client!");
            return false;
        }
        int updatedAge = atoi(readBuffer);
        if (updatedAge == 0)
        {
            // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid age^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("writing message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        customer.age = updatedAge;
        break;
    case 3:
        writeBytes = write(connFD, "New gender : ", strlen("New gender : "));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("getting response from client!");
            return false;
        }
        customer.gender = readBuffer[0];
        break;
    default:
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Invalid choice^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("writing message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    customerFileDescriptor = open(CUSTOMER_FILE, O_WRONLY);
    if (customerFileDescriptor == -1)
    {
        perror("opening customer file");
        return false;
    }
    offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (offset == -1)
    {
        perror("seeking to required customer record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("obtaining write lock on customer record!");
        return false;
    }

    writeBytes = write(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (writeBytes == -1)
    {
        perror("writing update customer info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLKW, &lock);

    close(customerFileDescriptor);

    writeBytes = write(connFD, "Changed Succesfully!^", strlen("Changed Succesfully!^"));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}


bool customer(int connFD){
    if (login_handler(false, connFD, &loggedInCustomer))
    {
        ssize_t writeBytes, readBytes;            
        char readBuffer[1000], writeBuffer[1000]; 

     
        key_t semKey = ftok(CUSTOMER_FILE, loggedInCustomer.account);

        union semun
        {
            int val; 
        } semSet;

        int semctlStatus;
        semIdentifier = semget(semKey, 1, 0); 
        if (semIdentifier == -1)
        {
            semIdentifier = semget(semKey, 1, IPC_CREAT | 0700);
            if (semIdentifier == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            semSet.val = 1; 
            semctlStatus = semctl(semIdentifier, 0, SETVAL, semSet);
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Welcome...");
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, "1. Get Customer Details\n2. Deposit Money\n3. Withdraw Money\n4. Get Balance\n5. Change Password\n6.exit");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing  to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for CUSTOMER_MENU");
                return false;
            }
            
            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, loggedInCustomer.id);
                break;
            case 2:
                deposit(connFD);
                break;
            case 3:
                withdraw(connFD);
                break;
            case 4:
                get_balance(connFD);
                break;
            case 5:
                change_password(connFD);
                break;
            default:
                writeBytes = write(connFD, "Logging out...$", strlen("Logging out...$"));
                return false;
            }
        }
    }
    else
    {
        write(connFD, "Login failed...$", strlen( "Login failed...$"));
        return false;
    }
    return true;
}

bool deposit(int connFD){
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Account account;
    account.accountNumber = loggedInCustomer.account;

    long int depositAmount = 0;

    // Lock the critical section
    struct sembuf semOp;
    lock_critical_section(&semOp);

    if (get_account_details(connFD, &account))
    {
        
        if (account.active)
        {

            writeBytes = write(connFD, "Deposit amount : ", strlen( "Deposit amount : "));
            if (writeBytes == -1)
            {
                perror("Error writing to client!");
                unlock_critical_section(&semOp);
                return false;
            }

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error reading from client!");
                unlock_critical_section(&semOp);
                return false;
            }

            depositAmount = atol(readBuffer);
            if (depositAmount >= 0)
            {

                account.balance += depositAmount;

                int accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
                off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Account), SEEK_SET);

                struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
                int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account file!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
                if (writeBytes == -1)
                {
                    perror("Error storing updated deposit money in account record!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                lock.l_type = F_UNLCK;
                fcntl(accountFileDescriptor, F_SETLK, &lock);

                write(connFD, "Added Succesfully!^", strlen("Added Succesfully!^"));
                read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

                get_balance(connFD);

                unlock_critical_section(&semOp);

                return true;
            }
            else
                writeBytes = write(connFD, "Invalid amount!^", strlen("Invalid amount!^"));
        }
        else
            write(connFD, "Account not active!^", strlen("Account not active!^"));
        read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

        unlock_critical_section(&semOp);
    }
    else
    {
        // FAIL
        unlock_critical_section(&semOp);
        return false;
    }
}

bool withdraw(int connFD){
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Account account;
    account.accountNumber = loggedInCustomer.account;

    long int withdrawAmount = 0;

    // Lock the critical section
    struct sembuf semOp;
    lock_critical_section(&semOp);

    if (get_account_details(connFD, &account))
    {
        if (account.active)
        {

            writeBytes = write(connFD, "Withdraw amount : ", strlen("Withdraw amount : "));
            if (writeBytes == -1)
            {
                perror("Error writing message to client!");
                unlock_critical_section(&semOp);
                return false;
            }

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error reading from client!");
                unlock_critical_section(&semOp);
                return false;
            }

            withdrawAmount = atol(readBuffer);

            if (withdrawAmount >= 0 && account.balance - withdrawAmount >= 0)
            {

                account.balance -= withdrawAmount;

                int accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
                off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Account), SEEK_SET);

                struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
                int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account record!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
                if (writeBytes == -1)
                {
                    perror("Error writing updated balance into account file!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                lock.l_type = F_UNLCK;
                fcntl(accountFileDescriptor, F_SETLK, &lock);

                write(connFD, "Withdraw success!^", strlen("Withdraw success!^"));
                read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

                get_balance(connFD);

                unlock_critical_section(&semOp);

                return true;
            }
            else
                writeBytes = write(connFD, "Invalid amount^", strlen( "Invalid amount^"));
        }
        else
            write(connFD, "Account is not active!^", strlen("Account is not active!^"));
        read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

        unlock_critical_section(&semOp);
    }
    else
    {
        // FAILURE while getting account information
        unlock_critical_section(&semOp);
        return false;
    }
}

bool get_balance(int connFD){
    char buffer[1000];
    struct Account account;
    account.accountNumber = loggedInCustomer.account;
    if (get_account_details(connFD, &account))
    {
        bzero(buffer, sizeof(buffer));
        if (account.active)
        {
            sprintf(buffer, "Balance : %ld  ₹ ^", account.balance);
            write(connFD, buffer, strlen(buffer));
        }
        else
            write(connFD, "Account is not active!^", strlen("Account is not active!^"));
        read(connFD, buffer, sizeof(buffer)); // Dummy read
    }
    else
    {
        // ERROR while getting balance
        return false;
    }
}

bool change_password(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000], hashedPassword[1000];

    char newPassword[1000];

    // Lock the critical section
    struct sembuf semOp = {0, -1, SEM_UNDO};
    int semopStatus = semop(semIdentifier, &semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }

    writeBytes = write(connFD, "Old password : ", strlen("Old password : "));
    if (writeBytes == -1)
    {
        perror("writing message to client!");
        unlock_critical_section(&semOp);
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading response from client");
        unlock_critical_section(&semOp);
        return false;
    }

    if (strcmp(readBuffer, loggedInCustomer.password) == 0)
    {
        // Password matches with old password
        writeBytes = write(connFD, "New password : ", strlen("New password : "));
        if (writeBytes == -1)
        {
            perror("Error writing message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading new password response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        strcpy(newPassword, readBuffer);

        writeBytes = write(connFD, "Re-enter password : ", strlen( "Re-enter password : "));
        if (writeBytes == -1)
        {
            perror("Error writing message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        if (strcmp(readBuffer, newPassword) == 0)
        {
            // New & reentered passwords match

            strcpy(loggedInCustomer.password, newPassword);

            int customerFileDescriptor = open(CUSTOMER_FILE, O_WRONLY);
            if (customerFileDescriptor == -1)
            {
                perror("Error opening customer file!");
                unlock_critical_section(&semOp);
                return false;
            }

            off_t offset = lseek(customerFileDescriptor, loggedInCustomer.id * sizeof(struct Customer), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
            int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            writeBytes = write(customerFileDescriptor, &loggedInCustomer, sizeof(struct Customer));
            if (writeBytes == -1)
            {
                perror("Error storing updated customer password into customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            lock.l_type = F_UNLCK;
            lockingStatus = fcntl(customerFileDescriptor, F_SETLK, &lock);

            close(customerFileDescriptor);

            writeBytes = write(connFD, "Password changed successfully!^", strlen("Password changed successfully!^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

            unlock_critical_section(&semOp);

            return true;
        }
        else
        {
            // New & reentered passwords don't match
            writeBytes = write(connFD, "Password dosent match!^", strlen("Password dosent match!^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        }
    }
    else
    {
        // Password doesn't match with old password
        writeBytes = write(connFD, "Incorrect password!", strlen("Incorrect password!"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    }

    unlock_critical_section(&semOp);

    return false;
}

bool lock_critical_section(struct sembuf *semOp){
    semOp->sem_flg = SEM_UNDO;
    semOp->sem_op = -1;
    semOp->sem_num = 0;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }
    return true;
}

bool unlock_critical_section(struct sembuf *semOp){
    semOp->sem_op = 1;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}

#endif