# proc-title-watch - Monitor Processes and their titles

**Currently implemented only for windows. mac/linux coming soon!**

This *native* module will search for processes that match a specified set of strings and return their PIDs and titles of their main visible window.

## Requirements

[node-gyp](https://github.com/nodejs/node-gyp) and it's requirements.

## Installing

`npm install https://github.com/joncppl/proc-title-watch`

## Usage

```javascript
var procWatch = require('../lib/proc-title-watch.js');

// an array of programs to search for
var searchList = [ "MPC", "VLC" ];

procWatch(searchList, function(error, info) {
  if (error) console.error(error);
  console.log(info);
});

// Example Output
// [ { pid: 1000, title: 'A.mp4' },
//   { pid: 1001, title: 'B.mkv' } ]
```



## Plans

- macOS & Linux support
- Events for changes, such as
  - new matching process launched
  - title changed
  - process closed

## Example Use Case

I developed this for a video catalogue application where I wanted to detect whether the user is currently watching a video and know when that video had ended so as to update the catalogue. The name of the video file is conveniently placed in the window title by most video players.
