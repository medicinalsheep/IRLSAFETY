/*
 * IRLSAFETY+ — OBS dock entry point (opens tray control panel).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "control_dock.hpp"
#include "onboarding_dialog.hpp"
#include "tray_panel.hpp"
#include "ui_theme.hpp"

#include "../irlsafety_shutdown.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

static IRLSafetyControlDock *g_control_dock = nullptr;

IRLSafetyControlDock::IRLSafetyControlDock(QWidget *parent) : QFrame(parent)
{
	irlsafety_ui::apply_dark_theme(this);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(10);

	auto *title = new QLabel(QStringLiteral("<b>IRLSAFETY+</b>"), this);
	title->setTextFormat(Qt::RichText);
	layout->addWidget(title);

	auto *hint = new QLabel(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Dock.TrayHint")), this);
	hint->setWordWrap(true);
	layout->addWidget(hint);

	auto *open_btn = new QPushButton(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Tray.OpenPanel")), this);
	connect(open_btn, &QPushButton::clicked, this, [] { irlsafety_tray_panel_show(); });
	layout->addWidget(open_btn);

	auto *guide_btn = new QPushButton(QString::fromUtf8(obs_module_text("IRLSAFETYPlus.Dock.ShowWalkthrough")), this);
	connect(guide_btn, &QPushButton::clicked, this, [] { irlsafety_onboarding_show(); });
	layout->addWidget(guide_btn);

	layout->addStretch(1);
}

IRLSafetyControlDock::~IRLSafetyControlDock() = default;

extern "C" void irlsafety_control_dock_register(void)
{
	if (g_control_dock)
		return;

	QWidget *main_window = static_cast<QWidget *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);
	g_control_dock = new IRLSafetyControlDock(main_window);
	obs_frontend_add_dock_by_id("irlsafety_plus_control", obs_module_text("IRLSAFETYPlus.Dock.Title"),
				    g_control_dock);
	obs_frontend_pop_ui_translation();
	irlsafety_onboarding_show_if_needed();
}

extern "C" void irlsafety_control_dock_unregister(void)
{
	if (!g_control_dock)
		return;

	obs_frontend_remove_dock("irlsafety_plus_control");
	if (!irlsafety_is_shutting_down())
		delete g_control_dock;
	g_control_dock = nullptr;
}