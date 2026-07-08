#include "customtitlebar.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>

CustomTitleBar::CustomTitleBar(const QString &title, QWidget *window, bool showMinimize, QWidget *parent)
    : QFrame(parent), m_window(window)
{
    setObjectName("customTitleBar");
    setFixedHeight(42);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 8, 0);
    layout->setSpacing(6);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("customTitleLabel");

    layout->addWidget(titleLabel);
    layout->addStretch();

    if (showMinimize) {
        auto *minButton = new QPushButton("-", this);
        minButton->setObjectName("titleButton");
        minButton->setFixedSize(34, 28);
        connect(minButton, &QPushButton::clicked, m_window, [window]() {
            window->showMinimized();
        });
        layout->addWidget(minButton);
    }

    auto *closeButton = new QPushButton("X", this);
    closeButton->setObjectName("closeTitleButton");
    closeButton->setFixedSize(34, 28);
    connect(closeButton, &QPushButton::clicked, m_window, [window]() {
        window->close();
    });
    layout->addWidget(closeButton);
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_window) {
        m_dragPosition = event->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && m_window) {
        m_window->move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}
