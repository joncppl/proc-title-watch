var procWatch = require('../lib/proc-title-watch.js');

console.log("Starting test...");

procWatch(["MPC", "VLC"], function(error, data) {
  if (error) console.log(error);
    console.log(data);
});
