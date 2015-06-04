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
void contextSawCheckRegisterConst(struct IRContext* context, const char* registerName, unsigned long long val);
void contextSawCheckRegister(struct IRContext* context, const char* registerName1, const char* registerName2);
void contextSawChecktState(struct IRContext* context, unsigned long long val);
void contextYYError(int line, int column, struct IRContext* context, const char* reason);
#ifdef __cplusplus
}
#endif
#endif /* IRCONTEXT_H */
