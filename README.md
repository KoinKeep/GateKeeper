# GateKeeper
### Dont trust, verify.

GateKeeper is a tool for verifying withdrawal transactions and more. It will verify that your Bitcoin wallet is not lying to or stealing from you.

GateKeeper should be used:
* After a withdrawal transaction is created.
* Before a transaction is published.

To ensure the transaction is not published, make your withdrawal transaction in an offline environment.

An input like this:
```
./gatekeeper -xpub xpub6DDpUQbZNypFjfd5EGeGvJdDUmTFqK3x3LcVBRj3nc6zfcJfWo9JGvPeeA8fXpRg1QANtwRfjAVCKhpC4bWy3AeHxXfDYyKvM5r9DAjoaF6 -tx 010000000119afd0bc3a654c3af937ec67f753fa9f9806ac46d6b84f3362dc671a0449e7d50000000000ffffffff024905020000000000160014987211ba9f3684c51c57841bfff316c1b612af7509301800000000001600148842945c0f057d09cbfdfd898a5b15d254b5580d6a5f0900 -aftx 02000000000101101b9492fb5a5f6aecc9ba8da71eb680b86b61e91f3ada0f5ec3639a90407adf0100000000feffffff026c361a0000000000160014cc9cb419157887286c11f2331b2bbd51ae34494868fe03000000000017a914f531f4d41989d55c15e62fb525b1421e7c572127870247304402206bafd0bcf32a5e0b9bfe869649359d292484e06e3fd10fa31f7699b5a68a73c502203b87014c096e2c18bdb00e471faef6d6286eeb9c937ae97967c508fd1020436f01210216a6bfd3e804aa30ce2f53b8427feeb0e8c503c1680f7f728cdeba9be5c87026c7580900
```

Creates outputs that look like this:
```
Building lookahead data of size 100. If the size is large or there are many xpubs & scripts, this may take a long time.
Finished building lookahead data.

Ver: 1, locktime: 614250, value: 1717586, fee: 282
[Input 1]
	Previous Transaction Hash: [d5e749041a67dc62334fb8d646ac06989ffa53f767ec37f93a4c653abcd0af19]
	Output Index: 0
	Sequence: ffffffff
[Output 1]
	Value: 0.00132425
	Script:  OP_FALSE PUSH(20) [987211ba9f3684c51c57841bfff316c1b612af75]
[Output 2]
	Value: 0.01585161
	Script:  OP_FALSE PUSH(20) [8842945c0f057d09cbfdfd898a5b15d254b5580d]

Receiving address bc1qnpeprw5lx6zv28zhssdlluckcxmp9tm4rpklvl is confirmed as a valid and owned p2wpkh address.

== Confirmed transaction details ==

0.01717868 external funding came from address(es):
* 0.01717868 bc1qejwtgxg40zrjsmq37ge3k2aa2xhrgj2gkvqv5t

0.01585161 sent to unowned address(es):
* 0.01585161 bc1q3ppfghq0q47snjlalkyc5kc46f2t2kqd89f2c9

-0.00000282 paid in transaction fees.

+0.00132425 received into address(es):
* +0.00132425 bc1qnpeprw5lx6zv28zhssdlluckcxmp9tm4rpklvl

Finished processing with no warnings.

== Confirmed transaction conclusion ==

Account balance adjustment is: +0.00132425.
```

After confirming the transaction details are what you expect, it is then safe to publish the withdrawal transaction. If any warnings or errors are found, the output will tell you them and you should *not* publish the transaction until they are resolved.

GateKeeper can also verify receiving addresses. A receiving address has its pubscript pulled and then an attempt is made to create the same receiving address derived from an xpub. The two addresses must match exactly or it will fail.
```
./gatekeeper -a bc1q89xk0cuh5n2dfurs5ncaq6d699esunh8qqmvfu -xpub xpub6DDpUQbZNypFjfd5EGeGvJdDUmTFqK3x3LcVBRj3nc6zfcJfWo9JGvPeeA8fXpRg1QANtwRfjAVCKhpC4bWy3AeHxXfDYyKvM5r9DAjoaF6
```

A transaction can be automatically verified without providing the funding transaction if you activate -node and -walletCreationTime. This will activate a lightweight bitcoin node that connects to the network and searches for the funding transactions.
```
./gatekeeper -node -walletCreationTime 1579094880 -xpub xpub6DDpUQbZNypFjfd5EGeGvJdDUmTFqK3x3LcVBRj3nc6zfcJfWo9JGvPeeA8fXpRg1QANtwRfjAVCKhpC4bWy3AeHxXfDYyKvM5r9DAjoaF6 -tx 02000000000101e6fee929628839ef215723e60c368cffd96805b93a0cd8b8539abe4a24c030ca010000000000000000023629000000000000160014273c2fa6875e93b40af5081605d508f7f726974ebfd40100000000001600145d145bc068c01ef295d7ad775852b949231184cd0248304502210086d7c26a106068c856af3538b40d10bdd7b69edb4ea14b7209273a8b58e0482f02206260c46bd03a836885ae0e305fec9e44fdff1d9eb38fd0e7c1a7da2fd291f56a0121027632cd3575d6ac28128aa5d1eac9167eee92f06a4f004efcdef9a1b502893e0100000000
```
This creates an output that looks like this:
```
Building lookahead data of size 15. If the size is large or there are many xpubs & scripts, this may take a long time.
Finished building lookahead data.

Ver: 2, locktime: 0, value: 130549
[Input 1]
	Previous Transaction Hash: [ca30c0244abe9a53b8d80c3ab90568d9ff8c360ce6235721ef39886229e9fee6]
	Output Index: 1
	Public Key: [027632cd3575d6ac28128aa5d1eac9167eee92f06a4f004efcdef9a1b502893e01]
	Signature: [304502210086d7c26a106068c856af3538b40d10bdd7b69edb4ea14b7209273a8b58e0482f02206260c46bd03a836885ae0e305fec9e44fdff1d9eb38fd0e7c1a7da2fd291f56a01]
	Witness stack: 2 element(s)
	Sequence: 00000000
[Output 1]
	Value: 0.00010550
	Script:  OP_FALSE PUSH(20) [273c2fa6875e93b40af5081605d508f7f726974e]
[Output 2]
	Value: 0.00119999
	Script:  OP_FALSE PUSH(20) [5d145bc068c01ef295d7ad775852b949231184cd]

WARNING: Unable to verify withdrawal amount. This knowledge is required to verify transaction fee amount.
Without this knowledge an attacker (or bug) may send an unknown amount of your funds to miners.
Possible remedies are:
1) Run again adding the -database parameter (if the funding transaction has been previously saved).
2) Run again but adding parameters -node and -walletCreationTime 1579094852 where 1579094852 is the unix time of the funding transaction (or earlier). This will connect to the bitcoin network searching for the funding transaction(s), saving them, and quiting when found.
3) Find the transaction and add it manually with -addFundingTransaction ABCD where ABCD is the transaction in hex. The transaction identifier is ca30c0244abe9a53b8d80c3ab90568d9ff8c360ce6235721ef39886229e9fee6.
4) If you are an advanced user, you can lookup the funding transaciton amount manually. The output index is 1.

Receiving address bc1qyu7zlf58t6fmgzh4pqtqt4gg7lmjd96wrmm6tc is confirmed as a valid and owned p2wpkh address.

== Confirmed transaction details ==

Withdrawing an unkown amount from the following address(es):
* WARNING: Unrenderable Amount WARNING: Unrenderable Address

0.00119999 sent to unowned address(es):
* 0.00119999 bc1qt529hsrgcq0099wh44m4s54efy33rpxdsve9wa

+0.00010550 received into address(es):
* +0.00010550 bc1qyu7zlf58t6fmgzh4pqtqt4gg7lmjd96wrmm6tc

Finished processing with 2 warnings. It is highly recommended that all warnings be fixed before publishing this transaction.

Resetting blocks due to new wallet creation time.
BitcoinSpoon node wallet initial creation time 1579094880
BitcoinSpoon node connecting to network... Control-C to exit.

46.226.18.135: Connection refused
rv1025.1blu.de: Connection refused
78-157-164-38-static.silesnet.cz: Connection refused
Node[tixy.fixelyserver.net] Requesting blocks past 00000000000000000015a7e162de31d8c40afa5452f6e3b5181bb98526bfc4c1
disconnecting from active node  [0x0]
Switched c3a0ac3.ip.berlin.ccc.de to be the active node->0x6120000373c0
Node checkup finished with 6 active connections
Node[c3a0ac3.ip.berlin.ccc.de] Requesting blocks past 00000000000000000015a7e162de31d8c40afa5452f6e3b5181bb98526bfc4c1
changeActiveNode: Only 1 node left! Can't cycle active node
Node checkup finished with 1 active connections
Node[185.64.116.15/Satoshi:0.17.1/] Requesting blocks past 00000000000000000015a7e162de31d8c40afa5452f6e3b5181bb98526bfc4c1
srv1.btcpaynow.net: Connection refused
Added 2000 blocks, new height: 612567
Node[185.64.116.15/Satoshi:0.17.1/] Requesting blocks past 0000000000000000001225a2c8d2f7cd499643af6ad47dd445ff5efe74f43e6b
Added 2000 blocks, new height: 614567
Node[185.64.116.15/Satoshi:0.17.1/] Requesting blocks past 000000000000000000049a4d7579cbf250427bbace6b9f37a7c0a6b079d1ca4a
Node[46.32.50.98/Satoshi:0.19.0.1/] Other side closed the connection
Added 994 blocks, new height: 615561
Blockchain headers are up to date!
Node[185.64.116.15/Satoshi:0.17.1/] Building bloom reqest from height 612815 of size 2747 [failures: 0/0]
Node checkup finished with 2 active connections
srv1.btcpaynow.net: Connection refused
78.108.187.246: Connection refused
process tx[d5ba025ca78ecf836909171d0557b535d90f09fbf72d73ce192bf228cf4aa4ab]
Node checkup finished with 2 active connections
btc.mum.jku.at: Connection refused
111.hosts212.ostrowski.pl: Connection refused
109.238.81.82: Network is unreachable
Node checkup finished with 2 active connections
185.21.216.134: Connection refused
pcgregr.fit.vutbr.cz: Connection refused
process tx[67bc77f9fb396dd277709ba317d6407b7ef0ba0a0fb8ee8c77c22e03a6f85e04]
Node checkup finished with 5 active connections
process tx[4e35d3b798f0ad063c1bca042209f66395e55e5d8e68d5892b04869ff2586b39]
process tx[2227fe2ce738636418b724f490e756207e6c8ef50e49d1fb7f3c6ff73d82beb3]
Node checkup finished with 6 active connections
process tx[909d09e5ca62834b7f4e2297987ef60edd8d3cd396ce87b230333ff5bb6a3666]
process tx[ca30c0244abe9a53b8d80c3ab90568d9ff8c360ce6235721ef39886229e9fee6]
Ver: 2, locktime: 0, value: 130549, fee: 509
[Input 1]
	Previous Transaction Hash: [ca30c0244abe9a53b8d80c3ab90568d9ff8c360ce6235721ef39886229e9fee6]
	Output Index: 1
	Public Key: [027632cd3575d6ac28128aa5d1eac9167eee92f06a4f004efcdef9a1b502893e01]
	Signature: [304502210086d7c26a106068c856af3538b40d10bdd7b69edb4ea14b7209273a8b58e0482f02206260c46bd03a836885ae0e305fec9e44fdff1d9eb38fd0e7c1a7da2fd291f56a01]
	Witness stack: 2 element(s)
	Sequence: 00000000
[Output 1]
	Value: 0.00010550
	Script:  OP_FALSE PUSH(20) [273c2fa6875e93b40af5081605d508f7f726974e]
[Output 2]
	Value: 0.00119999
	Script:  OP_FALSE PUSH(20) [5d145bc068c01ef295d7ad775852b949231184cd]

Receiving address bc1qyu7zlf58t6fmgzh4pqtqt4gg7lmjd96wrmm6tc is confirmed as a valid and owned p2wpkh address.

== Confirmed transaction details ==

-0.00131058 withdrawal out of address(es):
* -0.00131058 bc1qnhfzrxq6ktc3xukxdyqkxdmmggth44xfdgdky5

0.00119999 sent to unowned address(es):
* 0.00119999 bc1qt529hsrgcq0099wh44m4s54efy33rpxdsve9wa

-0.00000509 paid in transaction fees.

+0.00010550 received into address(es):
* +0.00010550 bc1qyu7zlf58t6fmgzh4pqtqt4gg7lmjd96wrmm6tc

Finished processing with no warnings.

== Confirmed transaction conclusion ==

Account balance adjustment is: -0.00120508.
```

See ./gatekeeper -h for all usage options

 Not being connected to internet is good -- keeping your storage in a faraday cage is even better.
