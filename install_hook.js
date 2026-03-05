const xahau = require('xahau');
const fs = require('fs');
const crypto = require('crypto');

const seed = 'yourSeed';
const network = "wss://xahau-test.net";

async function connectAndQuery() {
  const client = new xahau.Client(network);
  const my_wallet = xahau.Wallet.fromSeed(seed, {algorithm: 'secp256k1'});
  console.log(`Account: ${my_wallet.address}`);

  try {
    await client.connect();
    console.log('Connected to Xahau');

    const prepared = await client.autofill({
      "TransactionType": "SetHook",
      "Account": my_wallet.address,
      "Flags": 0,
      "Hooks": [
        {
          "Hook": {
            "HookHash": "3D3140B845F722C3832EDCF66CC7F60163292C90A7016B5FFE60B0D5D4DA537B",
            "HookOn": 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFFFBFFFFE', //Only Payments https://richardah.github.io/xrpl-hookon-calculator/
            "HookCanEmit": "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFFFBFFFFE", //Can emit Payments
            "HookNamespace": crypto.createHash('sha256').update('pct_split_payment').digest('hex').toUpperCase(),
            "Flags": 1,
            "HookParameters": [
              {
              "HookParameter": {
              "HookParameterName": "504354",
              "HookParameterValue": "1E"
              }
              },
              {
              "HookParameter": {
              "HookParameterName": "445354",
              "HookParameterValue": "4B50699E253C5098DEFE3A0872A79D129172F496"
              }
              }
              ]
          }
        }
      ],
    });

    const { tx_blob } = my_wallet.sign(prepared);
    const tx = await client.submitAndWait(tx_blob);
    console.log("Info tx:", JSON.stringify(tx, null, 2));
  } catch (error) {
    console.error('Error:', error);
  } finally {
    await client.disconnect();
    console.log('Disconnecting from Xahau node');
  }
}

connectAndQuery();
