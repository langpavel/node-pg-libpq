#ifndef NODE_PG_LIBPQ_EXEC
#define NODE_PG_LIBPQ_EXEC

#include "pg_libpq.h"

#define ASYNC_COMMAND(name) static void name(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args)

class ExecParams;

class ExecParamsResultHandler {
 public:
  virtual ~ExecParamsResultHandler();
  virtual Handle<Value> buildResult();

  ExecParams* ep;
};

class ExecParams : public PQAsync {
 public:
  ExecParams(Conn* conn, char* command, int paramsLen, char** params, Nan::Callback* callback);
  ~ExecParams();
  void Execute();
  static ExecParams* newInstance(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args);
  static void queue(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args);
  virtual Handle<Value> buildResult() {
    return resultHandler->buildResult();
  }
  void setResultHandler(ExecParamsResultHandler* value) {
    resultHandler = value;
    value->ep = this;
  }

 private:
  ExecParamsResultHandler* resultHandler;
  char* command;
  int paramsLen;
  char** params;
};

class PreparedStatement {
 public:
  ASYNC_COMMAND(prepare);
  ASYNC_COMMAND(execPrepared);
};

class CopyFromStream {
 public:
  static void queue(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args);
  static void putCopyData(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args);
  static void putCopyEnd(Conn* conn, const Nan::FunctionCallbackInfo<v8::Value>& args);
};

#endif
