#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "branch.h"
#include "error.h"
#include "debug.h"
#include <semaphore.h>

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  
  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  //
  int branchID = AccountNum_GetBranchID(accountNum);
  sem_wait(&(account->lockForAccount));
  sem_wait(&(bank->branches[branchID].branchLock));
  //
  Account_Adjust(bank,account, amount, 1);
  //
  sem_post(&(account->lockForAccount));
  sem_post(&(bank->branches[branchID].branchLock));
  //
  
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  
  //
  int branchID = AccountNum_GetBranchID(accountNum);
  sem_wait(&(account->lockForAccount));
  sem_wait(&(bank->branches[branchID].branchLock));
  //

  if (amount > Account_Balance(account)) {
    //
    sem_post(&(account->lockForAccount));
    sem_post(&(bank->branches[branchID].branchLock));
    //
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank,account, -amount, 1);
  //
  sem_post(&(account->lockForAccount));
  sem_post(&(bank->branches[branchID].branchLock));
  //
  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  
   assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(srcAccount)) {
    return ERROR_INSUFFICIENT_FUNDS;
  }

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  if(dstAccount == srcAccount){
    //sem_post(&(bank->check));
    return ERROR_SUCCESS;
  }
  if (amount > Account_Balance(srcAccount)) {
    //sem_post(&(bank->check));
    return ERROR_INSUFFICIENT_FUNDS;
  }
  
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
  int srcBranchID;
  int dstBranchID; 

  if(updateBranch){
    srcBranchID = AccountNum_GetBranchID(srcAccountNum);
    dstBranchID = AccountNum_GetBranchID(dstAccountNum);

    if (srcBranchID < dstBranchID) { 
      sem_wait(&(srcAccount->lockForAccount));
      sem_wait(&(dstAccount->lockForAccount));
      sem_wait(&(bank->branches[srcBranchID].branchLock));
      sem_wait(&(bank->branches[dstBranchID].branchLock));
    } else {
      sem_wait(&(dstAccount->lockForAccount));
      sem_wait(&(srcAccount->lockForAccount));
      sem_wait(&(bank->branches[dstBranchID].branchLock));
      sem_wait(&(bank->branches[srcBranchID].branchLock));
    }
  }else{
    if(srcAccount->accountNumber < dstAccount->accountNumber) {
      sem_wait(&(srcAccount->lockForAccount)); 
      sem_wait(&(dstAccount->lockForAccount));
    } else {
      sem_wait(&(dstAccount->lockForAccount));
      sem_wait(&(srcAccount->lockForAccount));
    }
  }


  Account_Adjust(bank, srcAccount, -amount, updateBranch);
  Account_Adjust(bank, dstAccount, amount, updateBranch);

  sem_post(&(dstAccount->lockForAccount));
  sem_post(&(srcAccount->lockForAccount));
  if(updateBranch){
    sem_post(&(bank->branches[dstBranchID].branchLock));
    sem_post(&(bank->branches[srcBranchID].branchLock));
  }
    
  //sem_post(&(bank->check));
  return ERROR_SUCCESS;
}
