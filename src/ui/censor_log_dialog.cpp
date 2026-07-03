/*
 * IRLSAFETY+ — censor activity log viewer.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "censor_log_dialog.hpp"

#include "../censor_log.h"
#include "../irlsafety_control.h"

#include <obs-module.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

static QString tr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QString kindLabel(irlsafety_censor_kind kind)
{
	switch (kind) {
	case IRLSAFETY_CENSOR_DETECT:
		return tr("IRLSAFETYPlus.CensorLog.KindDetect");
	case IRLSAFETY_CENSOR_OCR:
		return tr("IRLSAFETYPlus.CensorLog.KindOcr");
	case IRLSAFETY_CENSOR_SECURE:
		return tr("IRLSAFETYPlus.CensorLog.KindSecure");
	default:
		return QStringLiteral("?");
	}
}

static QString formatLog(obs_source_t *filter)
{
	irlsafety_censor_log_entry entries[IRLSAFETY_CENSOR_LOG_CAPACITY];
	size_t count;
	QString text;

	if (!filter)
		return tr("IRLSAFETYPlus.CensorLog.NoFilter");

	count = irlsafety_control_get_censor_log(filter, entries, IRLSAFETY_CENSOR_LOG_CAPACITY);
	if (count == 0)
		return tr("IRLSAFETYPlus.CensorLog.Empty");

	for (size_t i = 0; i < count; i++) {
		const irlsafety_censor_log_entry *e = &entries[i];
		text += QStringLiteral("[%1] frame %2 · %3 · %4 region(s) — %5\n")
				.arg((qulonglong)e->time_ms)
				.arg((qulonglong)e->frame_index)
				.arg(kindLabel(e->kind))
				.arg(e->region_count)
				.arg(QString::fromUtf8(e->detail));
	}

	return text;
}

class CensorLogDialog : public QDialog {
	Q_OBJECT

public:
	explicit CensorLogDialog(QWidget *parent, obs_source_t *filter) : QDialog(parent), filter_source(filter)
	{
		setWindowTitle(tr("IRLSAFETYPlus.CensorLog.Title"));
		resize(480, 360);

		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(10, 10, 10, 10);
		layout->setSpacing(8);

		auto *hint = new QLabel(tr("IRLSAFETYPlus.CensorLog.Hint"));
		hint->setWordWrap(true);
		hint->setStyleSheet(QStringLiteral("color: #9aa0a6; font-size: 11px;"));
		layout->addWidget(hint);

		log_view = new QPlainTextEdit();
		log_view->setReadOnly(true);
		log_view->setLineWrapMode(QPlainTextEdit::NoWrap);
		layout->addWidget(log_view, 1);

		auto *btn_row = new QHBoxLayout();
		auto *refresh_btn = new QPushButton(tr("IRLSAFETYPlus.CensorLog.Refresh"));
		auto *clear_btn = new QPushButton(tr("IRLSAFETYPlus.CensorLog.Clear"));
		btn_row->addWidget(refresh_btn);
		btn_row->addWidget(clear_btn);
		btn_row->addStretch(1);
		layout->addLayout(btn_row);

		auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
		layout->addWidget(buttons);

		connect(refresh_btn, &QPushButton::clicked, this, &CensorLogDialog::reload);
		connect(clear_btn, &QPushButton::clicked, this, &CensorLogDialog::clearLog);
		reload();
	}

private slots:
	void reload()
	{
		log_view->setPlainText(formatLog(filter_source));
	}

	void clearLog()
	{
		if (filter_source)
			irlsafety_control_clear_censor_log(filter_source);
		reload();
	}

private:
	obs_source_t *filter_source = nullptr;
	QPlainTextEdit *log_view = nullptr;
};

void irlsafety_censor_log_show(QWidget *parent, obs_source_t *filter)
{
	CensorLogDialog dlg(parent, filter);
	dlg.exec();
}

#include "censor_log_dialog.moc"