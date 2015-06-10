#include "exec.h"

ExecParamsResultHandler::~ExecParamsResultHandler() {}

Handle<Value> ExecParamsResultHandler::buildResult() {
  return ep->PQAsync::buildResult();
}

ExecParams::ExecParams(Conn* conn, char* command, int paramsLen, char** params, NanCallback* callback)
  : PQAsync(conn, callback), command(command), paramsLen(paramsLen), params(params) {
  resultHandler = (ExecParamsResultHandler*)NULL;
}

ExecParams::~ExecParams() {
  delete []command;
  if (params)
    conn->deleteUtf8StringArray(params, paramsLen);
  if (resultHandler)
    delete resultHandler;
}

void ExecParams::Execute() {
  if (params)
    setResult(PQexecParams(conn->pq, command,
                           paramsLen, NULL, params, NULL, NULL, 0));
  else
    setResult(PQexec(conn->pq, command));
}

ExecParams* ExecParams::newInstance(Conn* conn, const v8::FunctionCallbackInfo<v8::Value>& args) {
  ExecParams* async;
  char *command = conn->newUtf8String(args[0]);
  NanCallback* callback = new NanCallback(args[2].As<v8::Function>());

  if (args[1]->IsNull()) {
    async = new ExecParams(conn, command, 0, NULL, callback);
  } else {
    Local<Array> params = Local<Array>::Cast(args[1]);

    async = new ExecParams(conn, command, params->Length(),
                           conn->newUtf8StringArray(params),
                           callback);
  }
  return async;
}

void ExecParams::queue(Conn* conn, const v8::FunctionCallbackInfo<v8::Value>& args) {
  ExecParams* async = newInstance(conn, args);
  async->setResultHandler(new ExecParamsResultHandler());
  NanAsyncQueueWorker(async);
}

class CopyFromStreamResultHandler : public ExecParamsResultHandler {
  virtual Handle<Value> buildResult() {
    return NanNull();
  }
};

void CopyFromStream::queue(Conn* conn, const v8::FunctionCallbackInfo<v8::Value>& args) {
  ExecParams* async = ExecParams::newInstance(conn, args);
  async->setResultHandler(new CopyFromStreamResultHandler());
  conn->copy_inprogress = true;
  NanAsyncQueueWorker(async);
}

class PutCopyData : public PQAsync {
public:
  PutCopyData(Conn* conn, uint dataLength, char *data, NanCallback* callback) :
    PQAsync(conn, callback), dataLength(dataLength), data(data) {}

  virtual Handle<Value> buildResult() {
    return NanNull();
  }

  void Execute() {
    if (PQputCopyData(conn->pq, data, dataLength) == -1)
      SetErrorMessage(PQerrorMessage(conn->pq));
  }

  int dataLength;
  char *data;
};

void CopyFromStream::putCopyData(Conn* conn, const v8::FunctionCallbackInfo<v8::Value>& args) {
  char *str = Conn::newUtf8String(args[0]);
  uint len = Local<String>::Cast(args[0])->Length();
  PQAsync* async = new PutCopyData(conn, len, str, new NanCallback(args[1].As<v8::Function>()));
  NanAsyncQueueWorker(async);
}

class PutCopyEnd : public PQAsync {
public:
  PutCopyEnd(Conn* conn, char *error, NanCallback* callback) :
    PQAsync(conn, callback), error(error) {}

  virtual Handle<Value> buildResult() {
    return NanNull();
  }

  void Execute() {
    if (PQputCopyEnd(conn->pq, error) == -1)
      SetErrorMessage(PQerrorMessage(conn->pq));
    else if (error)
      SetErrorMessage(error);
    else
      setResult(PQgetResult(conn->pq));
  }

  char *error;
};

void CopyFromStream::putCopyEnd(Conn* conn, const v8::FunctionCallbackInfo<v8::Value>& args) {
  char *error = args[0]->IsNull() ? (char*)NULL : Conn::newUtf8String(args[0]);
  PQAsync* async = new PutCopyEnd(conn, error, new NanCallback(args[1].As<v8::Function>()));
  conn->copy_inprogress = false;
  NanAsyncQueueWorker(async);
}
