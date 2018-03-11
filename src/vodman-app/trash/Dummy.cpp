#include "Dummy.h"

#include <QDebug>

UrlFetchSucces::~UrlFetchSucces() {}
UrlFetchSucces::UrlFetchSucces(QObject* parent) : QObject(parent) {

}


void UrlFetchSucces::success(int id, QVariantMap data) {
    qDebug()  << id << data;
}
