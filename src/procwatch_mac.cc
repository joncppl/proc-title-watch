#include "nan.h"
#include "procwatch.h"

using namespace v8;
using namespace Nan;

void WatchWorker::Execute()
{
  ;// pass, unimplemented
}

void WatchWorker::HandleOKCallback()
{
  Nan::HandleScope scope;

  Local<Value> argv[] = {
    (New<v8::String>("Unimplemented")).ToLocalChecked(),
    Null()
  };

  callback->Call(2, argv);
}
