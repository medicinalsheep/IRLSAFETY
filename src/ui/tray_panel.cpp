/*
 * IRLSAFETY+ — system tray + floating control panel (dark mode, OBS-isolated).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "tray_panel.hpp"

#include "control_widget.hpp"
#include "onboarding_dialog.hpp"
#include "ui_theme.hpp"

#include "../irlsafety_shutdown.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

class IRLSafetyPanelWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit IRLSafetyPanelWindow(QWidget *parent = nullptr) : QMainWindow(parent)
	{
		setWindowTitle(QStringLiteral("IRLSAFETY+"));
		setMinimumSize(300, 380);
		resize(320, 420);
		setObjectName(QStringLiteral("irlsafetyPanelChrome"));
		irlsafety_ui::apply_dark_theme(this);

		auto *central = new QWidget(this);
		setCentralWidget(central);
		auto *layout = new QVBoxLayout(central);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		auto *title_bar = new QWidget(central);
		title_bar->setObjectName(QStringLiteral("irlsafetyTitleBar"));
		auto *title_layout = new QHBoxLayout(title_bar);
		title_layout->setContentsMargins(10, 6, 8, 6);

		auto *icon = new QLabel(title_bar);
		icon->setPixmap(QIcon(QStringLiteral(":/irlsafety/icons/appicon-tray.png")).pixmap(22, 22));
		title_layout->addWidget(icon);

		auto *titles = new QVBoxLayout();
		auto *title = new QLabel(QStringLiteral("<b>IRLSAFETY+</b>"), title_bar);
		title->setObjectName(QStringLiteral("irlsafetyTitle"));
		title->setTextFormat(Qt::RichText);
		auto *subtitle = new QLabel(QString::fromUtf8(PLUGIN_VERSION), title_bar);
		subtitle->setObjectName(QStringLiteral("irlsafetySubtitle"));
		titles->addWidget(title);
		titles->addWidget(subtitle);
		title_layout->addLayout(titles, 1);

		auto *hide_btn = new QPushButton(QStringLiteral("—"), title_bar);
		hide_btn->setFixedSize(32, 28);
		hide_btn->setToolTip(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Tray.HidePanel")));
		connect(hide_btn, &QPushButton::clicked, this, &IRLSafetyPanelWindow::hide);
		title_layout->addWidget(hide_btn);
		layout->addWidget(title_bar);

		layout->addWidget(new IRLSafetyControlWidget(central, true), 1);
	}
};

static QSystemTrayIcon *g_tray_icon = nullptr;
static IRLSafetyPanelWindow *g_panel_window = nullptr;

extern "C" void irlsafety_tray_panel_show(void)
{
	if (!g_panel_window)
		return;

	g_panel_window->show();
	g_panel_window->raise();
	g_panel_window->activateWindow();
}

extern "C" void irlsafety_tray_panel_register(void)
{
	if (g_tray_icon)
		return;

	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	QWidget *main_window = static_cast<QWidget *>(obs_frontend_get_main_window());
	g_panel_window = new IRLSafetyPanelWindow(main_window);

	g_tray_icon = new QSystemTrayIcon(main_window);
	g_tray_icon->setIcon(QIcon(QStringLiteral(":/irlsafety/icons/appicon-tray.png")));
	g_tray_icon->setToolTip(QStringLiteral("IRLSAFETY+ — Local privacy protection"));

	auto *menu = new QMenu(g_panel_window);
	menu->setStyleSheet(g_panel_window->styleSheet());
	auto *open_action = menu->addAction(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Tray.OpenPanel")));
	auto *walkthrough_action = menu->addAction(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Tray.Walkthrough")));
	menu->addSeparator();
	auto *hide_action = menu->addAction(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Tray.HidePanel")));

	g_tray_icon->setContextMenu(menu);

	QObject::connect(open_action, &QAction::triggered, [] { irlsafety_tray_panel_show(); });
	QObject::connect(walkthrough_action, &QAction::triggered, [] { irlsafety_onboarding_show(); });
	QObject::connect(hide_action, &QAction::triggered, [] {
		if (g_panel_window)
			g_panel_window->hide();
	});
	QObject::connect(g_tray_icon, &QSystemTrayIcon::activated, [](QSystemTrayIcon::ActivationReason reason) {
		if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
			irlsafety_tray_panel_show();
	});

	g_tray_icon->show();
}

extern "C" void irlsafety_tray_panel_unregister(void)
{
	if (g_tray_icon) {
		g_tray_icon->hide();
		if (!irlsafety_is_shutting_down())
			delete g_tray_icon;
		g_tray_icon = nullptr;
	}

	if (g_panel_window) {
		if (!irlsafety_is_shutting_down())
			delete g_panel_window;
		g_panel_window = nullptr;
	}
}

#include "tray_panel.moc"