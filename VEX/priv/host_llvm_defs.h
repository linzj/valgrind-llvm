#ifndef HOST_LLVM_DEFS_H
#define HOST_LLVM_DEFS_H

extern HInstrArray* iselSB_LLVM              ( IRSB*, 
                                             VexArch,
                                             VexArchInfo*,
                                             VexAbiInfo*,
                                             Int offs_Host_EvC_Counter,
                                             Int offs_Host_EvC_FailAddr,
                                             Bool chainingAllowed,
                                             Bool addProfInc,
                                             Addr64 max_ga );
#endif /* HOST_LLVM_DEFS_H */
