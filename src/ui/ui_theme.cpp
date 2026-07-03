/*
 * IRLSAFETY+ — dark theme for IRLSAFETY+ windows only (never touches OBS chrome).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ui_theme.hpp"

#include <QFile>
#include <QIODevice>
#include <QWidget>

namespace irlsafety_ui {

void apply_dark_theme(QWidget *root)
{
	if (!root)
		return;

	root->setObjectName(QStringLiteral("irlsafetyThemedRoot"));

	QFile theme_file(QStringLiteral(":/irlsafety/ui/dark_theme.qss"));
	if (!theme_file.open(QIODevice::ReadOnly))
		return;

	root->setStyleSheet(QString::fromUtf8(theme_file.readAll()));
}

} // namespace irlsafety_ui