#include "nan.h"
#include "procwatch.h"

using namespace v8;
using namespace Nan;

NAN_METHOD(AddProcListener)
{
  Nan::HandleScope scope;
  Local<Object> ss = info[0].As<Object>();

  std::vector<std::string> searchStrings;
  unsigned int size = info[1]->Uint32Value();
  for (unsigned int i = 0; i < size; ++i)
  {
    String::Utf8Value a(Get(ss, i).ToLocalChecked()->ToString());
    searchStrings.push_back(*a);
  }

  Callback *callback = new Callback(info[2].As<Function>());
  bool doDeepSearch = info[3].As<Boolean>()->BooleanValue();

  AsyncQueueWorker(new WatchWorker(callback, searchStrings, doDeepSearch));
}

NAN_MODULE_INIT(Init)
{
  Nan::Set(target, New<String>("AddProcListener").ToLocalChecked(),
    GetFunction(New<FunctionTemplate>(AddProcListener)).ToLocalChecked());
}

NODE_MODULE(procwatchtitle, Init)
