#pragma once

#include <QFrame>
#include <QPoint>

class CustomTitleBar : public QFrame {
public:
    explicit CustomTitleBar(const QString &title, QWidget *window, bool showMinimize, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QWidget *m_window = nullptr;
    QPoint m_dragPosition;
};
