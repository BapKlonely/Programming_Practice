#include "style.h"

QString appStyleSheet()
{
    return R"(
        QWidget {
            background: #f4f7fb;
            color: #172033;
            font-family: "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }

        QDialog, QMainWindow {
            background: #f4f7fb;
        }

        QFrame#customTitleBar {
            background: #0f172a;
            border: 0;
        }

        QLabel#customTitleLabel {
            color: #ffffff;
            background: transparent;
            font-size: 14px;
            font-weight: 700;
        }

        QPushButton#titleButton {
            background: transparent;
            color: #e2e8f0;
            border: 0;
            border-radius: 4px;
            min-height: 24px;
            padding: 0;
            font-size: 16px;
            font-weight: 700;
        }

        QPushButton#titleButton:hover {
            background: #334155;
        }

        QPushButton#closeTitleButton {
            background: transparent;
            color: #e2e8f0;
            border: 0;
            border-radius: 4px;
            min-height: 24px;
            padding: 0;
            font-size: 14px;
            font-weight: 700;
        }

        QPushButton#closeTitleButton:hover {
            background: #dc2626;
            color: #ffffff;
        }

        QLabel#heroTitle {
            color: #ffffff;
            background: transparent;
            font-size: 28px;
            font-weight: 700;
        }

        QLabel#heroSubtitle {
            color: #dbeafe;
            background: transparent;
            font-size: 14px;
        }

        QLabel#pageTitle {
            color: #0f172a;
            background: transparent;
            font-size: 28px;
            font-weight: 700;
        }

        QLabel#pageSubtitle {
            color: #64748b;
            background: transparent;
            font-size: 14px;
        }

        QLabel#sectionTitle {
            color: #0f172a;
            background: transparent;
            font-size: 17px;
            font-weight: 700;
        }

        QLabel#mutedLabel {
            color: #64748b;
            background: transparent;
        }

        QLabel#badge {
            color: #0f766e;
            background: #ccfbf1;
            border: 1px solid #99f6e4;
            border-radius: 8px;
            padding: 5px 10px;
            font-weight: 600;
        }

        QFrame#heroPanel {
            background: #12355b;
            border-radius: 8px;
        }

        QFrame#panel, QGroupBox {
            background: #ffffff;
            border: 1px solid #dbe4f0;
            border-radius: 8px;
        }

        QGroupBox {
            margin-top: 18px;
            padding: 16px 14px 14px 14px;
            font-weight: 700;
            color: #0f172a;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            background: #ffffff;
        }

        QFrame#metricCard {
            background: #ffffff;
            border: 1px solid #dbe4f0;
            border-radius: 8px;
        }

        QLabel#metricNumber {
            color: #0f766e;
            background: transparent;
            font-size: 26px;
            font-weight: 700;
        }

        QLabel#metricLabel {
            color: #64748b;
            background: transparent;
            font-size: 13px;
        }

        QLineEdit, QDateEdit, QSpinBox, QDoubleSpinBox {
            background: #ffffff;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            min-height: 30px;
            padding: 5px 8px;
            selection-background-color: #0f766e;
        }

        QLineEdit:focus, QDateEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #0f766e;
            background: #f8fffd;
        }

        QPushButton {
            background: #0f766e;
            color: #ffffff;
            border: 0;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
            min-height: 30px;
        }

        QPushButton:hover {
            background: #0d9488;
        }

        QPushButton:pressed {
            background: #115e59;
        }

        QPushButton:disabled {
            background: #cbd5e1;
            color: #64748b;
        }

        QPushButton#secondaryButton {
            background: #e2e8f0;
            color: #172033;
        }

        QPushButton#secondaryButton:hover {
            background: #cbd5e1;
        }

        QPushButton#dangerButton {
            background: #dc2626;
        }

        QPushButton#dangerButton:hover {
            background: #ef4444;
        }

        QPushButton#menuButton {
            background: #ffffff;
            color: #0f172a;
            border: 1px solid #dbe4f0;
            border-left: 5px solid #0f766e;
            border-radius: 8px;
            text-align: left;
            padding: 18px 20px;
            font-size: 18px;
            font-weight: 700;
        }

        QPushButton#menuButton:hover {
            background: #ecfeff;
            border-color: #67e8f9;
            border-left: 5px solid #0891b2;
        }

        QPushButton#menuButton:disabled {
            background: #f1f5f9;
            color: #94a3b8;
            border-left: 5px solid #cbd5e1;
        }

        QPushButton#logoutButton {
            background: #334155;
            color: #ffffff;
            border-radius: 8px;
            font-size: 16px;
            padding: 14px 18px;
        }

        QPushButton#logoutButton:hover {
            background: #475569;
        }

        QTableWidget {
            background: #ffffff;
            alternate-background-color: #f8fafc;
            border: 1px solid #dbe4f0;
            border-radius: 8px;
            gridline-color: #e2e8f0;
            selection-background-color: #ccfbf1;
            selection-color: #0f172a;
        }

        QHeaderView::section {
            background: #12355b;
            color: #ffffff;
            border: 0;
            border-right: 1px solid #294b70;
            padding: 8px;
            font-weight: 700;
        }

        QTableCornerButton::section {
            background: #12355b;
            border: 0;
        }
    )";
}
