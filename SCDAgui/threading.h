#ifndef THREADING_H
#define THREADING_H

#include <QObject>
#include <QThread>
#include <QDebug>

class THREADING : public QObject
{
    Q_OBJECT

public:
    explicit THREADING(QObject *parent = nullptr);
    ~THREADING();

signals:
    void test();

public slots:

};

#endif // THREADING_H
