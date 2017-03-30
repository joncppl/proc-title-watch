var procWatch = require('../build/Release/proc-title-watch.node');

var AddProcListener = function(searchStrings, callback) {
  if (typeof searchStrings == "string") {
    searchStrings = [ searchStrings ];
  }
  doDeepSearch = false; // currently disable feature

  procWatch.AddProcListener(searchStrings, searchStrings.length, callback, doDeepSearch);
}

module.exports = AddProcListener;
