#include "threading.h"

THREADING::THREADING(QObject *parent) : QObject(parent) {}

THREADING::~THREADING() {}

void THREADING::test()
{
    qDebug() << "Signal from " << QThread::currentThreadId();
}

