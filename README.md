Admin : 
    login : a
    passw : 1

customer: 
    login : pra-0
    passw : pra21

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
2.  normal user{
        > Deposit
        > Withdraw
        > Balance Enquiry
        > Password Change
        > View details
        > Exit
    }
    joint account user{
        > implement file locking
        > to view the account details then read lock is to be set
        > to withdraw or deposit then write lock should be used to protect the critical data section.

    }
    administrator{
        > add
        > delete
        > modify
        > search for a specific account details.
    }

Process Management
File Management
File Locking
Multithreading
Inter Process Communication
