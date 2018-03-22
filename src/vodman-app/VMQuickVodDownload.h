/* The MIT License (MIT)
 *
 * Copyright (c) 2018 Jean Gressmann <jean@0x42.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "VMVodFileDownload.h"


class VMQuickVodDownload : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(VMVodFileDownload data READ data NOTIFY dataChanged)
//    Q_PROPERTY(float progress READ progress NOTIFY progressChanged)
//    Q_PROPERTY(VMVod vod READ vod CONSTANT)
public:
    ~VMQuickVodDownload();
    explicit VMQuickVodDownload(QObject* parent = Q_NULLPTR);

//    int handle() const;
    VMVodFileDownload data() const { return m_Download; }
    void setData(const VMVodFileDownload& value);
//    float progress() const { return m_Download.progress(); }


signals:
    void dataChanged();
//    void progressChanged();

private:
    VMVodFileDownload m_Download;
};
