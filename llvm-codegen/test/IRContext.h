#ifndef IRCONTEXT_H
#define IRCONTEXT_H
#ifdef __cplusplus
extern "C" {
#endif
struct IRContext {
    void* m_scanner;
};

void contextSawIRExit(struct IRContext* context, unsigned long long);
void contextSawEof(struct IRContext* context);
void contextSawError(struct IRContext* context);
void contextYYError(int line, int column, struct IRContext* context, const char* reason);
#ifdef __cplusplus
}
#endif
#endif /* IRCONTEXT_H */
