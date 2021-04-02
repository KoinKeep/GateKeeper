//
//  main.c
//  BitcoinSpoon
//
//  Created by Dustin Dettmer on 1/27/20.
//  Copyright © 2020 BitcoinSpoon. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Database.h"
#include "KeyManager.h"
#include "BasicStorage.h"
#include "TransactionTracker.h"
#include "Transaction.h"
#include "Database.h"
#include "NodeManager.h"
#include "Notifications.h"

static volatile int keepRunning = 1;

static void intHandler(int dummy)
{
    keepRunning = 0;
}

static void blockchainSyncChange(Dict dict)
{
    if(nodeManager.blockchainSynced) {

        printf("ERROR: Blockchain finished syncing without finding funding transactions.\n");
        printf("Here are some things that may correct the problem:\n");
        printf("1) Set -walletCreationTime to a unix timestamp before the time the funding transctions (when in doubt, try an earlier timestamp).\n");
        printf("2) Manually download funding transactions and add them with -addFundingTransaction. Look earlier in the log for which transaction identifiers are needed.\n");

        keepRunning = 0;
    }
}

// Returns 0 on succcess
static int processTransactionStr(const char *transactionStr, int onlyIfValid);

static int validatePubScript(Data pubScript, const char *addressStr);

static int quietMode = 0;
static int testnet = 0;

int main(int argc, char **argv)
{
    signal(SIGINT, intHandler);

    const char *xpubStr[100] = {0};
    const char *xpubNameStr[100] = {0};
    const char *xpubDeriveStr[100] = {0};
    
    memset(xpubNameStr, 0, 100);
    memset(xpubDeriveStr, 0, 100);
    
    int xpubCount = -1;
    const char *xpubRemoveStr = NULL;
    int listxPubs = 0;
    const char *scriptMultisigStr = NULL;
    const char *scriptVaultStr = NULL;
    const char *scriptNameStr = NULL;
    int listScripts = 0;
    const char *transactionStr = NULL;
    const char *addressStr = NULL;
    const char *addFundingTransactionStr = NULL;
    const char *directory = "/tmp";
    int lookaheadCount = -1;
    int walletCreationTimeOverride = 0;
    int basicStorage = 0;
    int keyManager = 0;
    int enableDatabase = 0;
    int transactionTracker = 0;
    int node = 0;

    const int noNodeLookaheadDefault = 100;

    if(argc == 1) {

        static char *help[2] = { "", "-h" };

        help[0] = argv[0];

        argv = help;
    }

    while(*argv && *++argv) {

        if(strstr(*argv, "-h")) {

            printf("bitcoinSpoon is primarily used for verifying transaction and receiving addresses.\n");
            printf("First, register your xpub(s). These are saved along with other metadata in a directory\n");
            printf("specified by the -directory command.\n");
            printf("\n");
            printf("After registering, use the -transaction and -address commands to verify them. Be sure to\n");
            printf("use the same -directory that you used when registering your xpub(s).\n");
            printf("\n");
            printf("A secondary feature is running as an SPV node to download funding transactions. Funding\n");
            printf("transactions are required to verify the mining fee amount spent by new transactions.\n");
            printf("\n");
            printf("Register xpub:\n");
            printf("\n");
            printf("-xpub xpub661My...                     Parses and saves the supplied xpub\n");
            printf("-xpubName myxPubName                   Optionally name the imported xpub (default is a generated uuid)\n");
            printf("-xpubRemove myxPubName                 Removes a given xpub by name\n");
            printf("-xpubDerive \"m/45'/0..\"                Derives the given xpub by derivation path before storage. Also outputs resulting xpub.\n");
            printf("-listxPubs                             Lists all registered xpubs\n");
            printf("\n");
            printf("Register multisig scripts:\n");
            printf("\n");
            printf("-scriptMultisig xPubName1,xPubName2,.. Adds multisig script where n is 2/3rds of m, rounding down.\n");
            printf("-scriptVault xPubName1,xPubName2,..    Adds KoinKeep 'vault' style multisig script.\n");
            printf("-scriptName myVaultName                Optionally name the registered script (default is a generated uuid)\n");
            printf("-listScripts                           Lists all registered multisig scripts\n");
            printf("\n");
            printf("Transaction and Address Parsing:\n");
            printf("\n");
            printf("-transaction ABCDEF123                 Parses transasction from hex value and analyzes\n");
            printf("-tx                                    Shorthand for -transaction\n");
            printf("-address bc1123445678                  Parses receiving address and analyzes it\n");
            printf("-a bc1123445678                        Shorthand for -address\n");
            printf("-addFundingTransaction ABCDEF123       Parses transasction from hex value and saves it for reference\n");
            printf("-aftx ABCDEF123                        Shorthand for -addFundingTransaction\n");
            printf("\n");
            printf("Settings:\n");
            printf("\n");
            printf("-testnet                               (default %d)\n", testnet);
            printf("-quietMode 1                           (default %d) Options are 0 (loud), 1, 2, 3, 4 (quietest)\n", quietMode);
            printf("-directory \"somePath\"                  (default \"%s\")\n", directory);
            printf("-lookAheadCount 100000                 (default %d)\n", TransactionTrackerBloomaheadCount);
            printf("-walletCreationTime 1580162685         (default timestamp of first time keyManager module is initialized)\n");
            printf("\n");
            printf("Passing a transaction and not enabling node will change the default lookAheadCount to %d.\n", noNodeLookaheadDefault);
            printf("When scanning for funding transactions, be sure walletCreationTime is set to *before* the funding transaction time.\n");
            printf("\n");
            printf("Modules:\n");
            printf("\n");
            printf("-basicStorage                          (default %d)\n", basicStorage);
            printf("-keyManager                            (default %d)\n", keyManager);
            printf("-database                              (default %d)\n", enableDatabase);
            printf("-transactionTracker                    (default %d)\n", transactionTracker);
            printf("-node                                  (default %d)\n", node);
            printf("\n");
            printf("Enabling a module enables all dependent modules as well.\n");

            return 1;
        }

        else if(*argv == strstr(*argv, "-testnet"))
            testnet = 1;

        else if(*argv == strstr(*argv, "-quietMode"))
            quietMode = atoi(*++argv ?: "-1");

        else if(*argv == strstr(*argv, "-directory"))
            directory = *++argv;

        else if(*argv == strstr(*argv, "-address") || 0 == strcmp(*argv, "-a"))
            addressStr = *++argv;

        else if(*argv == strstr(*argv, "-addFundingTransaction") || 0 == strcmp(*argv, "-aftx"))
            addFundingTransactionStr = *++argv;

        else if(*argv == strstr(*argv, "-lookAheadCount") || *argv == strstr(*argv, "-lookaheadCount"))
            lookaheadCount = atoi(*++argv ?: "-1");

        else if(*argv == strstr(*argv, "-walletCreationTime"))
            walletCreationTimeOverride = atoi(*++argv ?: "-1");

        else if(*argv == strstr(*argv, "-transaction") || *argv == strstr(*argv, "-tx"))
            transactionStr = *++argv;

        else if(0 == strcmp(*argv, "-xpub")) {

            if(xpubCount >= 99) {

                printf("-xpub limit is 100 items.\n");
                return 1;
            }

            xpubStr[++xpubCount] = *++argv;
        }

        else if(*argv == strstr(*argv, "-xpubName"))
            xpubNameStr[xpubCount] = *++argv;

        else if(*argv == strstr(*argv, "-xpubRemove"))
            xpubRemoveStr = *++argv;
        
        else if(*argv == strstr(*argv, "-xpubDerive"))
            xpubDeriveStr[xpubCount] = *++argv;

        else if(*argv == strstr(*argv, "-listxPubs"))
            listxPubs = 1;

        else if(*argv == strstr(*argv, "-scriptVault"))
            scriptVaultStr = *++argv;

        else if(*argv == strstr(*argv, "-scriptMultisig"))
            scriptMultisigStr = *++argv;

        else if(*argv == strstr(*argv, "-scriptName"))
            scriptNameStr = *++argv;

        else if(*argv == strstr(*argv, "-listScripts"))
            listScripts = 1;

        else if(*argv == strstr(*argv, "-keyManager"))
            keyManager = 1;

        else if(*argv == strstr(*argv, "-database"))
            enableDatabase = 1;

        else if(*argv == strstr(*argv, "-transactionTracker"))
            transactionTracker = 1;

        else if(*argv == strstr(*argv, "-node"))
            node = 1;

        else if(*argv == strstr(*argv, "-basicStorage"))
            basicStorage = 1;
    }

    xpubCount++;

    if(testnet < 0) {

        printf("Invalid value for testnet.\n");
        return 1;
    }

    if(quietMode < 0) {

        printf("Invalid value for quietMode.\n");
        return 1;
    }

    for(int i = 0; i < xpubCount; i++) {

        if(xpubStr[i] && !base58Dencode(xpubStr[i]).bytes) {

            printf("Invalid value for xpub %i.\n", i + 1);
            return 1;
        }
    }

    if(addFundingTransactionStr && fromHex(addFundingTransactionStr).length * 2 != strlen(addFundingTransactionStr)) {

        printf("Invalid value for addFundingTransaction.\n");
        return 1;
    }

    if(walletCreationTimeOverride < 0) {

        printf("Invalid walletCreationTime.\n");
        return 1;
    }

    if(!directory) {

        printf("Invalid value for directory.\n");
        return 1;
    }

    if(keyManager < 0) {

        printf("Invalid value for keyManager.\n");
        return 1;
    }

    if(enableDatabase < 0) {

        printf("Invalid value for database.\n");
        return 1;
    }

    if(transactionTracker < 0) {

        printf("Invalid value for transactionTracker.\n");
        return 1;
    }

    if(node < 0) {

        printf("Invalid value for node.\n");
        return 1;
    }

    if(addFundingTransactionStr) {

        enableDatabase = 1;
    }

    if(transactionStr || addressStr) {

        transactionTracker = 1;
    }

    if(xpubCount || listxPubs || scriptVaultStr || scriptMultisigStr || listScripts) {

        keyManager = 1;
    }

    if(node) {

        basicStorage = 1;
        keyManager = 1;
        enableDatabase = 1;
        transactionTracker = 1;
    }

    if(keyManager) {

        basicStorage = 1;
    }

    if(enableDatabase) {

        basicStorage = 1;
    }

    if(transactionTracker) {

        basicStorage = 1;
        keyManager = 1;
    }

    if(lookaheadCount < 0 && (transactionStr || addressStr) && !node) {

        lookaheadCount = noNodeLookaheadDefault;
    }

    if(!quietMode) {

        printf("Startup settings: ");
        printf("testnet[%d] ", testnet);
        printf("directory[%s] ", directory);
        printf("lookaheadCount[%d] ", lookaheadCount);
        printf("walletCreationTime[%d] ", walletCreationTimeOverride);
        printf("basicStorage[%d] ", basicStorage);
        printf("keyManager[%d] ", keyManager);
        printf("database[%d] ", enableDatabase);
        printf("transactionTracker[%d] ", transactionTracker);
        printf("node[%d] ", node);
        printf("\n\n");
    }

    databaseRootPath = directory;
    keyManagerKeyDirectory = directory;

    if(lookaheadCount > 0)
        TransactionTrackerBloomaheadCount = lookaheadCount;

    DataTrackPush();

    BTCUtilStartup();

    if(basicStorage) {

        basicStorageSetup(StringF("%s/bs.basicStorage", directory));
        bsSave("testnet", DataInt(testnet));
    }

    time_t walletCreationTime = walletCreationTimeOverride;

    if(keyManager) {

        KMInit();
        KMSetTestnet(&km, testnet);

        if(!walletCreationTime) {

            Data d = bsLoad("walletCreationTime");
            walletCreationTime = DataGetLong(d);
        }

        if(!walletCreationTime) {

            walletCreationTime = time(0);
            bsSave("walletCreationTime", DataLong(walletCreationTime));
        }
    }

    int keysDidChange = 0;

    for(int i = 0; i < xpubCount; i++) {

        keysDidChange = 1;

        Data hdWalletData = base58Dencode(xpubStr[i]);

        String name = makeUuid();

        if(xpubNameStr[i])
            name = StringNew(xpubNameStr[i]);
        
        if(xpubDeriveStr[i]) {
            
            hdWalletData = hdWallet(hdWalletData, xpubDeriveStr[i]);
            
            printf("xpub[%d] derivation \"%s\" result: %s\n", i, xpubDeriveStr[i], base58Encode(hdWalletData).bytes);
        }

        KMSetHDWalletForUUID(&km, hdWalletData, name, KeyManagerHdWalletTypeManual);

        printf("Registered xpub with name %s\n", name.bytes);
    }

    if(xpubCount)
        printf("\n");

    if(xpubRemoveStr) {

        keysDidChange = 1;

        KMSetHDWalletForUUID(&km, DataNull(), StringNew(xpubRemoveStr), 0);

        printf("Removed xpub named %s\n", xpubRemoveStr);
        printf("\n");
    }

    if(listxPubs) {

        Dict wallets = KMKnownHDWallets(&km);

        printf("List of all registered xpubs:\n");

        FORINDICT(item, wallets) {

            Dict details = DictDeserialize(item->value);

            Data hdWalletData = DictGetS(details, knownHDWalletDataKey);

            printf("%s -> %s\n", item->key.bytes, base58Encode(hdWalletData).bytes);
        }

        printf("\n");
    }

    if(scriptMultisigStr) {

        keysDidChange = 1;

        Datas hdWallets = DatasNew();
        Datas components = StringComponents(StringNew(scriptMultisigStr), ',');

        FORDATAIN(name, components) {

            Data hdWalletData = KMHdWalletFrom(&km, StringNew(name->bytes));

            if(!hdWalletData.bytes) {

                printf("Unable to find xpub named %s. You must first add it with the -xpub command.", name->bytes);
                printf("\n");
                return 1;
            }

            hdWallets = DatasAddCopy(hdWallets, hdWalletData);
        }

        String scriptName = StringNew(scriptNameStr ?: makeUuid().bytes);

        if(DatasHasMatchingData(KMVaultNames(&km), scriptName)) {

            printf("Already have a script named %s.\n", scriptName.bytes);
            printf("\n");
        }
        else if(hdWallets.count < 1) {

            printf("Invalid number of keys for vault script (need at least 1).\n");
            printf("\n");

            return 1;
        }
        else if(hdWallets.count){

            KMAddMultisig(&km, hdWallets, scriptName);

            printf("Added new multisig script named %s.\n", scriptName.bytes);
            printf("\n");
        }
    }

    if(scriptVaultStr) {

        keysDidChange = 1;

        Datas hdWallets = DatasNew();
        Datas components = StringComponents(StringNew(scriptVaultStr), ',');

        FORDATAIN(name, components) {

            Data hdWalletData = KMHdWalletFrom(&km, StringNew(name->bytes));

            if(!hdWalletData.bytes) {

                printf("Unable to find xpub named %s. You must first add it with the -xpub command.", name->bytes);
                printf("\n");
                return 1;
            }

            hdWallets = DatasAddCopy(hdWallets, hdWalletData);
        }

        String vaultName = StringNew(scriptNameStr ?: makeUuid().bytes);

        if(DatasHasMatchingData(KMVaultNames(&km), vaultName)) {

            printf("Already have a script named %s.\n", vaultName.bytes);
            printf("\n");
        }
        else if(hdWallets.count < 2) {

            printf("Invalid number of keys for vault script (need at least 2).\n");
            printf("\n");

            return 1;
        }
        else if(hdWallets.count){

            Data masterHdWallet = DatasAt(hdWallets, 0);

            hdWallets = DatasRemoveIndexTake(hdWallets, 0);

            // First hdWallet is the master hdWallet
            KMAddVaultObserver(&km, masterHdWallet, hdWallets, vaultName);

            printf("Added new vault script named %s.\n", vaultName.bytes);
            printf("\n");
        }
    }

    if(listScripts) {

        Datas/*String*/ vaultNames = KMVaultNames(&km);

        printf("List of all registered custom scripts:\n");

        if(!vaultNames.count)
            printf("No scripts.\n\n");

        for(int i = 0; i < vaultNames.count; i++) {

            String name = DatasAt(vaultNames, i);

            Datas/*Data*/ hdWallets = DatasDeserialize(DatasAt(KMVaultAllHdWallets(&km), i));

            printf("Script Name: %s\n", name.bytes);

            FORDATAIN(data, hdWallets)
                printf("%s\n", base58Encode(*data).bytes);

            printf("\n");
        }
    }

    if(enableDatabase) {

        database = DatabaseNew();
    }

    if(addFundingTransactionStr) {

        Data data = fromHex(addFundingTransactionStr);

        DatabaseAddTransaction(&database, data, NULL);
    }

    if(transactionTracker) {

        tracker = TTNew(testnet);

        const char *key = "TransactionTrackerBloomaheadCount";

        int oldValue = DataGetInt(bsLoad(key));

        if(keysDidChange || TTBloomFilterNeedsUpdate(&tracker) || oldValue < TransactionTrackerBloomaheadCount) {

            printf("Building lookahead data of size %d. If the size is large or there are many xpubs & scripts, this may take a long time.\n", TransactionTrackerBloomaheadCount);

            TTUpdateBloomFilter(&tracker);

            printf("Finished building lookahead data.\n");
            printf("\n");

            bsSave(key, DataInt(TransactionTrackerBloomaheadCount));
        }
        else {

            printf("Loading lookahead data.\n");

            TTKeysAndKeyHashes(&tracker);

            printf("Finished loading lookahead data.\n");
            printf("\n");
        }
    }

    if(addressStr) {

        Data pubScript = addressToPubScript(StringNew(addressStr));

        return validatePubScript(pubScript, addressStr);
    }

    if(transactionStr) {

        int result = processTransactionStr(transactionStr, 0);

        if(result < 1 || !node)
            return result == 0 ? 0 : 1;
    }

    DataTrackPop();

    if(node) {

        DataTrackPush();

        nodeManager = NodeManagerNew(walletCreationTime);

        const char *key = "NodeManagerWalletCreationTimeCache";

        if(DataGetLong(bsLoad(key)) != walletCreationTime) {

            printf("Resetting blocks due to new wallet creation time.\n");

            DatabaseResetAllBlocks(&database);
            TTSetBloomFilterDlHeight(&tracker, 0);

            bsSave(key, DataLong(walletCreationTime));
        }

        nodeManager.automaticallyPublishWaitingTransactions = 0;

        nodeManager.testnet = testnet;

        printf("BitcoinSpoon node wallet initial creation time %ld\n", walletCreationTime);
        printf("BitcoinSpoon node connecting to network... Control-C to exit.\n");
        printf("\n");

        NotificationsAddListener(NodeManagerBlockchainSyncChange, blockchainSyncChange);

        NodeManagerConnectNodes(&nodeManager);

        DataTrackPop();

        while(keepRunning) {

            DataTrackPush();

            NodeManagerProcessNodes(&nodeManager);

            int result = processTransactionStr(transactionStr, 1);

            if(result == 0)
                return 0;

            NotificationsProcess();

            DataTrackPop();

            usleep(1000 * 1000);
        }

        DataTrackPop();
    }

    BTCUtilShutdown();

    return 10;
}

static int processTransactionStr(const char *transactionStr, int onlyIfValid)
{
    Data data = fromHex(transactionStr);

    if(!data.bytes) {

        printf("Invalid hex data for transaction");
        return -1;
    }

    Transaction tx = TransactionNew(data);

    FORIN(TransactionInput, input, tx.inputs) {

        Transaction *t = TTTransactionForTxid(&tracker, input->previousTransactionHash);
        TransactionOutput *output = NULL;

        if(t)
            output = TransactionOutputOrNilAt(t, input->outputIndex);

        if(output)
            TransactionInputSetFundingOutput(input, output->value, DataCopyData(output->script));
        else if(onlyIfValid)
            return -1;
    }

    if(quietMode < 2)
        printf("%s\n", TransactionDescription(tx).bytes);

    int warnings = 0;
    int errors = 0;

    if(TTInterestingTransaction(&tracker, &tx)) {

        int externalWithdrawlCount = 0;
        uint64_t externalWithdrawlAmount = 0;
        uint64_t withdrawlAmount = 0;
        uint64_t depositAmount = 0;
        uint64_t externalDepositAmount = 0;

        FORIN(TransactionInput, input, tx.inputs) {

            if(TTInterestingInput(&tracker, input)) {

                if(!input->fundingOutput.value) {

                    if(quietMode < 4) {

                        printf("WARNING: Unable to verify withdrawal amount. This knowledge is required to verify transaction fee amount.\n");
                        printf("Without this knowledge an attacker (or bug) may send an unknown amount of your funds to miners.\n");
                        printf("Possible remedies are:\n");
                        printf("1) Run again adding the -database parameter (if the funding transaction has been previously saved).\n");
                        printf("2) Run again but adding parameters -node and -walletCreationTime 1579094852 where 1579094852 is the unix time of the funding transaction (or earlier). This will connect to the bitcoin network searching for the funding transaction(s), saving them, and quiting when found.\n");
                        printf("3) Find the transaction and add it manually with -addFundingTransaction ABCD where ABCD is the transaction in hex. The transaction identifier is %s.\n", toHex(DataFlipEndianCopy(input->previousTransactionHash)).bytes);
                        printf("4) If you are an advanced user, you can lookup the funding transaciton amount manually. The output index is %d.\n\n", input->outputIndex);
                    }

                    warnings++;

                    withdrawlAmount = -1;
                }

                if(withdrawlAmount != -1)
                    withdrawlAmount += input->fundingOutput.value;
            }
            else {

                externalWithdrawlCount++;

                if(!input->fundingOutput.value) {

                    printf("NOTICE: Unable to verify incoming amount. This knowledge is required to verify transaction fee amount.\n");
                    printf("However since balance it is not coming from an owned address, this is only a notice and not a warning.\n");
                    printf("Possible remedies are:\n");
                    printf("1) Find the transaction and add it manually with -addFundingTransaction ABCD where ABCD is the transaction in hex. The transaction identifier is %s.\n", toHex(DataFlipEndianCopy(input->previousTransactionHash)).bytes);
                    printf("2) If you are an advanced user, you can lookup the funding transaciton amount manually. The output index is %d.\n\n", input->outputIndex);

                    externalWithdrawlAmount = -1;
                }

                if(externalWithdrawlAmount != -1)
                    externalWithdrawlAmount += input->fundingOutput.value;
            }
        }

        FORIN(TransactionOutput, output, tx.outputs) {

            if(TTInterestingOutput(&tracker, output)) {

                if(0 != validatePubScript(output->script, pubScriptToAddress(output->script).bytes)) {

                    errors++;
                }

                depositAmount += output->value;
            }
            else {

                externalDepositAmount += output->value;
            }
        }

        if(quietMode < 4) {

            if(!errors)
                printf("== Confirmed transaction details ==\n");
            else
                printf("== ERROR PROCESSING TRANSACTION DO NOT PUBLISH ==\n");

            printf("\n");

            if(externalWithdrawlCount) {

                if(externalWithdrawlAmount == -1)
                    printf("External funding from address(es):\n");
                else
                    printf("%s external funding came from address(es):\n", formatBitcoinAmount(externalWithdrawlAmount).bytes);

                FORIN(TransactionInput, input, tx.inputs) {

                    if(!TTInterestingInput(&tracker, input)) {

                        String address = pubScriptToAddress(input->fundingOutput.script);

                        if(!address.bytes)
                            address = StringNew("Unknown Address");

                        String amount = formatBitcoinAmount(input->fundingOutput.value);

                        if(!input->fundingOutput.value)
                            amount = StringNew("Unknown Amount");

                        printf("* %s %s\n", amount.bytes, address.bytes);
                    }
                }

                printf("\n");
            }

            if(withdrawlAmount) {

                if(withdrawlAmount == -1)
                    printf("Withdrawing an unkown amount from the following address(es):\n");
                else
                    printf("%s withdrawal out of address(es):\n", formatBitcoinAmount(-withdrawlAmount).bytes);

                FORIN(TransactionInput, input, tx.inputs) {

                    if(TTInterestingInput(&tracker, input)) {

                        String address = pubScriptToAddress(input->fundingOutput.script);

                        if(!address.bytes) {

                            warnings++;
                            address = StringNew("WARNING: Unrenderable Address");
                        }

                        String amount = formatBitcoinAmount(-input->fundingOutput.value);

                        if(!input->fundingOutput.value)
                            amount = StringNew("WARNING: Unrenderable Amount");

                        printf("* %s %s\n", amount.bytes, address.bytes);
                    }
                }

                printf("\n");
            }

            if(externalDepositAmount) {

                printf("%s sent to unowned address(es):\n", formatBitcoinAmount(externalDepositAmount).bytes);

                FORIN(TransactionOutput, output, tx.outputs) {

                    if(!TTInterestingOutput(&tracker, output)) {

                        String address = pubScriptToAddress(output->script);

                        if(!address.bytes) {

                            warnings++;
                            address = StringNew("WARNING: Unrenderable Address");
                        }

                        printf("* %s %s\n", formatBitcoinAmount(output->value).bytes, address.bytes);
                    }
                }

                printf("\n");
            }

            if(externalDepositAmount != -1 && depositAmount != -1 && withdrawlAmount != -1 && externalWithdrawlAmount != -1) {

                int64_t amount = externalWithdrawlAmount + withdrawlAmount;

                amount -= externalDepositAmount + depositAmount;

                printf("%s paid in transaction fees.\n", formatBitcoinAmount(-amount).bytes);
                printf("\n");
            }

            if(depositAmount) {

                printf("+%s received into address(es):\n", formatBitcoinAmount(depositAmount).bytes);

                FORIN(TransactionOutput, output, tx.outputs) {

                    if(TTInterestingOutput(&tracker, output)) {

                        String address = pubScriptToAddress(output->script);

                        if(!address.bytes) {

                            warnings++;
                            address = StringNew("WARNING: Unrenderable Address");
                        }

                        printf("* +%s %s\n", formatBitcoinAmount(output->value).bytes, address.bytes);
                    }
                }

                printf("\n");
            }
        }

        if(errors) {

            printf("ERROR: Finished processing with %d error%s and %d warning%s.\n", warnings, warnings == 1 ? "" : "s", errors, errors == 1 ? "" : "s");
            printf("DO NOT PUBLISH TRANSACTION UNTIL ERRORS ARE RESOLVED.\n");
        }
        else if(warnings)
            printf("Finished processing with %d warning%s. It is highly recommended that all warnings be fixed before publishing this transaction.\n", warnings, warnings == 1 ? "" : "s");
        else {

            printf("Finished processing with no warnings.\n");
            printf("\n");

            printf("== Confirmed transaction conclusion ==\n");
            printf("\n");

            int64_t amount = depositAmount - withdrawlAmount;
            printf("Account balance adjustment is: %s%s.\n", amount > 0 ? "+" : "", formatBitcoinAmount(amount).bytes);
        }

        printf("\n");

        return warnings + errors;
    }
    else {

        printf("ERROR: Unable to locate any keys involved in this transactions. Please update your public keys.\n");
        return -1;
    }
}

static int validatePubScript(Data pubScript, const char *addressStr)
{
    if(!DataEqual(pubScriptToAddress(pubScript), StringNew(addressStr))) {

        printf("ERROR: Receiving address %s is not a recognized valid address format.\n", addressStr);
        return 1;
    }

    TransactionOutput output = {0};

    output.script = pubScript;

    Datas datas = TTInterestingOutputMatches(&tracker, &output);

    if(datas.count) {

        if(datas.count != 1) {

            printf("ERROR: Receiving address %s format unknown -- multiple matches are not expected.\n\n", addressStr);
            return 1;
        }

        Data hash = DatasAt(datas, 0);

        Data item = TTPubKeyOrScriptForKnownHash(&tracker, hash);

        if(!item.bytes) {

            printf("ERROR: Unable to lookup pubKey or script for hash %s\n\n", toHex(hash).bytes);
            return 1;
        }

        Dict candidates = DictNew();

        if(testnet) {

            DictAdd(&candidates, p2pkhAddressTestNet(item), StringNew("p2pkh testnet"));
            DictAdd(&candidates, p2shAddressTestNet(item), StringNew("p2sh testnet"));
            DictAdd(&candidates, p2wpkhAddressTestNet(item), StringNew("p2wpkh testnet"));
            DictAdd(&candidates, p2wshAddressTestNet(item), StringNew("p2wsh testnet"));
        }
        else {

            DictAdd(&candidates, p2pkhAddress(item), StringNew("p2pkh"));
            DictAdd(&candidates, p2shAddress(item), StringNew("p2sh"));
            DictAdd(&candidates, p2wpkhAddress(item), StringNew("p2wpkh"));
            DictAdd(&candidates, p2wshAddress(item), StringNew("p2wsh"));
        }

        // TODO: Add p2sh wrapped p2wpkh/p2wsh

        String result = DictGetS(candidates, addressStr);

        if(result.bytes) {

            printf("Receiving address %s is confirmed as a valid and owned %s address.\n\n", addressStr, result.bytes);
            return 0;
        }
        else {

            printf("ERROR: Receiving address %s is not a recognized address type.\n\n", addressStr);
            return 2;
        }
    }
    else {

        printf("ERROR: Receiving address %s is not a valid owned address.\n", addressStr);
        printf("You may be using an incorrect key or the lookahead count may be too low.\n\n");
        return 1;
    }
}
