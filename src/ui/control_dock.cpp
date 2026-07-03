/*
 * IRLSAFETY+ — OBS dock host for shared control widget.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "control_dock.hpp"
#include "control_widget.hpp"
#include "onboarding_dialog.hpp"

#include "../irlsafety_shutdown.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QVBoxLayout>

static IRLSafetyControlDock *g_control_dock = nullptr;

IRLSafetyControlDock::IRLSafetyControlDock(QWidget *parent) : QFrame(parent)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(new IRLSafetyControlWidget(this));
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