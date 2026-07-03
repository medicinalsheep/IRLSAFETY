/*
 * IRLSAFETY+ — deferred OBS UI registration (after frontend is ready).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "control_dock.hpp"
#include "tray_panel.hpp"

#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QTimer>

#include <cstdlib>

extern "C" void irlsafety_schedule_ui_registration(void)
{
	if (const char *disable = std::getenv("IRLSAFETY_NO_UI")) {
		if (disable[0] == '1' || disable[0] == 'y' || disable[0] == 'Y') {
			obs_log(LOG_INFO, "IRLSAFETY+: UI registration disabled (IRLSAFETY_NO_UI)");
			return;
		}
	}

	QTimer::singleShot(500, []() {
		if (!obs_frontend_get_main_window())
			return;

		obs_log(LOG_INFO, "IRLSAFETY+: registering dock and tray (deferred)");
		irlsafety_control_dock_register();
		irlsafety_tray_panel_register();
		obs_log(LOG_INFO, "IRLSAFETY+: Tray panel ready — click the IRLSAFETY+ icon in the system tray");
		obs_log(LOG_INFO, "IRLSAFETY+: OBS dock also available under Docks → IRLSAFETY+ Control");
	});
}