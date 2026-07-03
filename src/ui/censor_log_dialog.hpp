/*
 * IRLSAFETY+ — censor activity log viewer.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

struct obs_source;

class QWidget;

void irlsafety_censor_log_show(QWidget *parent, struct obs_source *filter);