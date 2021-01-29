/*
 * Copyright (c) 2020 Plexim GmbH.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 3, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <QtCore/QDebug>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QLinearGradient>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

#include "CpuLoad.h"


class LoadAwareLabel: public QLabel
{
public:
    LoadAwareLabel(): QLabel()
    {
        setActive(false);
        setLoad(0);
    }

    void setLoad(double aLoad)
    {
        if (aLoad > 0)
        {
            if (!isActive())
                setActive(true);
        }
        else
        {
            if (isActive())
                setActive(false);
        }

        mLoad = aLoad;
        updateLabel();
    }

    double getLoad() { return mLoad; }

protected:
    virtual void updateLabel() {}

    bool isActive() { return mIsActive; }

    void setActive(bool aActive)
    {
        if (aActive)
            setStyleSheet("");
        else
            setStyleSheet("* {color: gray}");

        mIsActive = aActive;
    }

private:
    double mLoad;
    bool mIsActive;
};


class CpuIdLabel: public LoadAwareLabel
{
public:
    CpuIdLabel(): LoadAwareLabel() {}
};


class CpuLoadLabel: public LoadAwareLabel
{
public:
    CpuLoadLabel(): LoadAwareLabel()
    {
        updateLabel();
    }

protected:
    void updateLabel() override
    {
        int load = getLoad() * 100;
        auto text = QString("%1%").arg(load, 3);

        setText(text);
    }
};


class CpuLoadBar: public QWidget
{
public:
    CpuLoadBar(): QWidget(), mLoad(0)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        updateBar();
    }

    void setLoad(double aLoad)
    {
        mLoad = aLoad;
        updateBar();
    }

protected:
    void updateBar()
    {
        QWidget::update();
    }

    void paintEvent(QPaintEvent *aEvent) override
    {
        Q_UNUSED(aEvent);

        QPainter painter(this);

        if (mLoad == 0)
            painter.setPen(Qt::gray);

        auto rect = QRect(0, 0, size().width(), size().height());
        rect.adjust(+1, +2, -1, -2);
        painter.drawRect(rect);

        if (mLoad > 0)
        {
            auto fillRect = rect.adjusted(+2, +2, -1, -1);
            fillRect.setWidth(fillRect.width() * qMin(mLoad, 1.0));
            painter.fillRect(fillRect, QColor::fromHsv(0, 0, 0xB0));
        }
    }

private:
    double mLoad;
};


CpuLoad::CpuLoad(const QString &aLabel, QWidget *aParent)
: QWidget(aParent)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    auto *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mIdLabel = new CpuIdLabel;
    mIdLabel->setText(aLabel);;
    mainLayout->addWidget(mIdLabel);

    mLoadBar = new CpuLoadBar;
    mainLayout->addWidget(mLoadBar);

    mLoadLabel = new CpuLoadLabel;
    mLoadLabel->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(mLoadLabel);

    setLayout(mainLayout);
}


void CpuLoad::setLoad(double aLoad)
{
    mIdLabel->setLoad(aLoad);
    mLoadLabel->setLoad(aLoad);
    mLoadBar->setLoad(aLoad);
}
