#ifndef IRCONTEXT_H
#define IRCONTEXT_H
#ifdef __cplusplus
extern "C" {
#endif
struct IRContext {
    void* m_scanner;
};

void contextSawIRExit(struct IRContext* context, unsigned long long);
void contextSawRegisterInit(struct IRContext* context, const char* registerName, unsigned long long val);
void contextSawRegisterInitMemory(struct IRContext* context, const char* registerName, unsigned long long size, unsigned long long val);
void contextSawCheckRegisterConst(struct IRContext* context, const char* registerName, unsigned long long val);
void contextSawCheckRegister(struct IRContext* context, const char* registerName1, const char* registerName2);
void contextSawChecktState(struct IRContext* context, unsigned long long val1, unsigned long long val2);
void contextSawIRWr(struct IRContext* context, const char* id, void* expr);
void contextSawIRPutExpr(struct IRContext* context, unsigned long long where, void* expr);
void* contextNewConstExpr(struct IRContext* context, unsigned long long val);
void* contextNewRdTmpExpr(struct IRContext* context, const char* id);
void contextYYError(int line, int column, struct IRContext* context, const char* reason);
#ifdef __cplusplus
}
#endif
#endif /* IRCONTEXT_H */
