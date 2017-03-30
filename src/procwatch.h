#ifndef SRC_PROCWATCH_H_
#define SRC_PROCWATCH_H_

#include <string>
#include <vector>
#include <map>

class WatchWorker : public Nan::AsyncWorker {
  public:
    WatchWorker(Nan::Callback *callback, std::vector<std::string> searchStrings, bool doDeepSearch)
    : AsyncWorker(callback), searchStrings(searchStrings), doDeepSearch(doDeepSearch){};

    ~WatchWorker() {};
    void Execute();
    void HandleOKCallback();

  private:
    BOOL ProcessSearch(/*IN*/std::vector<std::string> search, /*IN*/BOOL doDeepSearch, /*OUT*/std::vector<DWORD> *pids);
    BOOL DeepProcessSearch(/*IN*/DWORD pid, /*IN*/std::vector<std::string>search);
    BOOL GetVisibleWindowTitle(/*IN*/DWORD pid, /*OUT*/char *title, /*IN*/UINT titlelen);
    void printError(TCHAR* msg);

    std::vector<std::string> searchStrings;
    std::map<unsigned long, std::string> pid_title_map;
    bool doDeepSearch;
    bool isError;
    std::string errorMsg = "";

};

#endif  // SRC_PROCWATCH_H_
