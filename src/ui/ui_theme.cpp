/*
 * IRLSAFETY+ — dark theme loader for Qt UI surfaces.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ui_theme.hpp"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QIODevice>
#include <QPalette>
#include <QWidget>

namespace irlsafety_ui {

void apply_dark_theme(QWidget *root)
{
	if (!root)
		return;

	QFile theme_file(QStringLiteral(":/irlsafety/ui/dark_theme.qss"));
	if (theme_file.open(QIODevice::ReadOnly)) {
		const QString sheet = QString::fromUtf8(theme_file.readAll());
		if (qApp)
			qApp->setStyleSheet(sheet);
		else
			root->setStyleSheet(sheet);
	}

	QPalette palette;
	palette.setColor(QPalette::Window, QColor(18, 20, 26));
	palette.setColor(QPalette::WindowText, QColor(232, 234, 237));
	palette.setColor(QPalette::Base, QColor(26, 31, 40));
	palette.setColor(QPalette::AlternateBase, QColor(30, 36, 48));
	palette.setColor(QPalette::Text, QColor(232, 234, 237));
	palette.setColor(QPalette::Button, QColor(30, 36, 48));
	palette.setColor(QPalette::ButtonText, QColor(232, 234, 237));
	palette.setColor(QPalette::Highlight, QColor(62, 207, 142));
	palette.setColor(QPalette::HighlightedText, QColor(18, 20, 26));
	if (qApp)
		qApp->setPalette(palette);
}

} // namespace irlsafety_ui