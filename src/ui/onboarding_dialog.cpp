/*
 * IRLSAFETY+ — first-run setup walkthrough.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "onboarding_dialog.hpp"
#include "ui_theme.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <util/config-file.h>

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

static const char *CONFIG_SECTION = "IRLSAFETY";
static const char *CONFIG_WALKTHROUGH_DONE = "walkthrough_complete";

static QString trKey(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

class IRLSafetyOnboardingDialog : public QDialog {
	Q_OBJECT

public:
	explicit IRLSafetyOnboardingDialog(QWidget *parent = nullptr) : QDialog(parent)
	{
		setWindowTitle(trKey("IRLSAFETYPlus.Walkthrough.Title"));
		setMinimumWidth(520);
		setModal(false);
		irlsafety_ui::apply_dark_theme(this);

		auto *layout = new QVBoxLayout(this);
		layout->setSpacing(10);

		step_label = new QLabel();
		step_label->setStyleSheet(QStringLiteral("color: #8ab4f8; font-weight: 600;"));
		layout->addWidget(step_label);

		body = new QTextBrowser();
		body->setOpenExternalLinks(false);
		body->setFrameShape(QFrame::NoFrame);
		layout->addWidget(body, 1);

		auto *buttons = new QHBoxLayout();
		skip_btn = new QPushButton(trKey("IRLSAFETYPlus.Walkthrough.Skip"));
		back_btn = new QPushButton(trKey("IRLSAFETYPlus.Walkthrough.Back"));
		next_btn = new QPushButton(trKey("IRLSAFETYPlus.Walkthrough.Next"));
		finish_btn = new QPushButton(trKey("IRLSAFETYPlus.Walkthrough.Finish"));
		finish_btn->setDefault(true);
		finish_btn->hide();

		buttons->addWidget(skip_btn);
		buttons->addStretch(1);
		buttons->addWidget(back_btn);
		buttons->addWidget(next_btn);
		buttons->addWidget(finish_btn);
		layout->addLayout(buttons);

		connect(skip_btn, &QPushButton::clicked, this, &IRLSafetyOnboardingDialog::skipWalkthrough);
		connect(back_btn, &QPushButton::clicked, this, &IRLSafetyOnboardingDialog::goBack);
		connect(next_btn, &QPushButton::clicked, this, &IRLSafetyOnboardingDialog::goNext);
		connect(finish_btn, &QPushButton::clicked, this, &IRLSafetyOnboardingDialog::finishWalkthrough);

		showPage(0);
	}

private:
	static constexpr int PAGE_COUNT = 8;

	QLabel *step_label = nullptr;
	QTextBrowser *body = nullptr;
	QPushButton *skip_btn = nullptr;
	QPushButton *back_btn = nullptr;
	QPushButton *next_btn = nullptr;
	QPushButton *finish_btn = nullptr;
	int page = 0;

	static void markWalkthroughComplete()
	{
		config_t *cfg = obs_frontend_get_profile_config();
		if (!cfg)
			return;
		config_set_bool(cfg, CONFIG_SECTION, CONFIG_WALKTHROUGH_DONE, true);
		config_save(cfg);
	}

	static bool walkthroughComplete()
	{
		config_t *cfg = obs_frontend_get_profile_config();
		if (!cfg)
			return false;
		return config_get_bool(cfg, CONFIG_SECTION, CONFIG_WALKTHROUGH_DONE);
	}

	void showPage(int index)
	{
		static const char *step_keys[PAGE_COUNT] = {
			"IRLSAFETYPlus.Walkthrough.Step01",
			"IRLSAFETYPlus.Walkthrough.Step02",
			"IRLSAFETYPlus.Walkthrough.Step03",
			"IRLSAFETYPlus.Walkthrough.Step04",
			"IRLSAFETYPlus.Walkthrough.Step05",
			"IRLSAFETYPlus.Walkthrough.Step06",
			"IRLSAFETYPlus.Walkthrough.Step07",
			"IRLSAFETYPlus.Walkthrough.Step08",
		};
		static const char *body_keys[PAGE_COUNT] = {
			"IRLSAFETYPlus.Walkthrough.Body01",
			"IRLSAFETYPlus.Walkthrough.Body02",
			"IRLSAFETYPlus.Walkthrough.Body03",
			"IRLSAFETYPlus.Walkthrough.Body04",
			"IRLSAFETYPlus.Walkthrough.Body05",
			"IRLSAFETYPlus.Walkthrough.Body06",
			"IRLSAFETYPlus.Walkthrough.Body07",
			"IRLSAFETYPlus.Walkthrough.Body08",
		};

		page = index;
		step_label->setText(trKey(step_keys[page]));
		body->setHtml(trKey(body_keys[page]));
		back_btn->setEnabled(page > 0);
		next_btn->setVisible(page < PAGE_COUNT - 1);
		finish_btn->setVisible(page == PAGE_COUNT - 1);
	}

	void goBack()
	{
		if (page > 0)
			showPage(page - 1);
	}

	void goNext()
	{
		if (page < PAGE_COUNT - 1)
			showPage(page + 1);
	}

	void skipWalkthrough()
	{
		markWalkthroughComplete();
		reject();
	}

	void finishWalkthrough()
	{
		markWalkthroughComplete();
		accept();
	}

public:
	static bool isComplete()
	{
		return walkthroughComplete();
	}
};

static void showOnboardingDialog()
{
	QWidget *main_window = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!main_window)
		return;

	obs_frontend_push_ui_translation(obs_module_get_string);
	IRLSafetyOnboardingDialog dialog(main_window);
	dialog.exec();
	obs_frontend_pop_ui_translation();
}

extern "C" void irlsafety_onboarding_show_if_needed(void)
{
	if (IRLSafetyOnboardingDialog::isComplete())
		return;

	QTimer::singleShot(900, []() {
		if (!IRLSafetyOnboardingDialog::isComplete())
			showOnboardingDialog();
	});
}

extern "C" void irlsafety_onboarding_show(void)
{
	showOnboardingDialog();
}

#include "onboarding_dialog.moc"