#pragma once

#include <QObject>
#include <QVariantMap>

class UrlFetchSucces : public QObject
{
    Q_OBJECT
public:
    ~UrlFetchSucces();
    explicit UrlFetchSucces(QObject* parent = Q_NULLPTR);

public slots:
    void success(int id, QVariantMap data);
};

