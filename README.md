Admin : 
    login : a
    passw : 1

customer: 
    user    : pass
    pra-0   : pra21

joint acc:
    user    : pass
    zoro-1  : zoro22
    sanji-2 : sanji21

client -> server (socket programming) : 

Server{
    > maintains the data base
    > service multiple clients concurrently. 
} 

Client{
    > connect to the server
    > access their specific account details.
}


1.  registration / login
    - normal user{
        - Deposit
        - Withdraw
        - Balance Enquiry
        - Password Change
        - View details
        - Exit
    }
    - joint account user{
        - implement file locking
        - to view the account details then read lock is to be set
        - to withdraw or deposit then write lock should be used to protect the critical data section.

    }
    - administrator{
        - add
        - delete
        - modify
        - search for a specific account details.
    }

File Management
File Locking
Inter Process Communication
