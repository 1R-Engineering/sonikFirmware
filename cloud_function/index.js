// // Create and Deploy Your First Cloud Functions
// // https://firebase.google.com/docs/functions/write-firebase-functions
//


const functions = require("firebase-functions"),
  PubSub = require(`@google-cloud/pubsub`),
  admin = require("firebase-admin");

const app = admin.initializeApp();
const firestore = app.firestore();
var num = 0;

firestore.settings({ timestampsInSnapshots: true });

const auth = app.auth();

exports.deviceState = functions.pubsub.topic('soniktop').onPublish(async (message) => {
    const deviceId = message.attributes.deviceId;
    const deviceRef = firestore.doc(`device-configs/${deviceId}`);
    try {
      await deviceRef.update({ 'sensors': message.getElementByID, 'online': true, 'timestamp' : admin.firestore.Timestamp.now() });
      console.log(`State updated for ${deviceId}`);
    } catch (error) {
      console.error(`${deviceId} not yet registered to a user`, error);
    }
});

exports.updateDevice = functions.firestore.document('pubSubTest/{docID}').onWrite((change, context) => {
  const topic = pubsub.topic('test');
  const otherBuffer = Buffer.from('this is the message');

  const callback = (err, messageId) => {
    if (err) {
      console.error(`error encountered during publish - ${err}`);
    } else {
      console.log(`Message ${messageId} published.`);
    }
  };

  // this worked, but the function is listed as deprecated
  topic.publish(otherBuffer, callback);

  // this did not work - {otherBuffer} is from  the doc
  // but I also tried without the curly braces and received the same error.
  //topic.publishMessage({otherBuffer}, callback);

  return null;
});