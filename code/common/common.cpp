#include "common.h"

void registerCommonMetaTypes() {
    // Qt template types.
    qRegisterMetaType< QList<QStringList> >("QList<QStringList>");
    qRegisterMetaType< QList<float> >("QList<float>");
    // Common custom types.
    qRegisterMetaType<Time>("Time");
    qRegisterMetaType<RawTransaction>("RawTransaction");
    qRegisterMetaType<Granularity>("Granularity");
    qRegisterMetaType<Bucket>("Bucket");
    qRegisterMetaType<BatchMetadata>("BatchMetadata");
    qRegisterMetaType<Batch<RawTransaction> >("Batch<RawTransaction>");
}
