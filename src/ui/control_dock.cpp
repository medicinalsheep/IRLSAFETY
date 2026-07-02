/*
 * IRLSAFETY+ — OBS control dock (model + training hub).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "control_dock.hpp"
#include "onboarding_dialog.hpp"

#include "../irlsafety_control.h"
#include "../filter_settings.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <cstring>

static IRLSafetyControlDock *g_control_dock = nullptr;

static QString tr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QString statusColor(bool ok)
{
	return ok ? QStringLiteral("#3ecf8e") : QStringLiteral("#f0a030");
}

IRLSafetyControlDock::IRLSafetyControlDock(QWidget *parent) : QFrame(parent)
{
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(8, 8, 8, 8);
	outer->setSpacing(6);

	auto *title_row = new QHBoxLayout();
	auto *title = new QLabel(QStringLiteral("<b>IRLSAFETY+</b> ") + QString::fromUtf8(PLUGIN_VERSION));
	title->setTextFormat(Qt::RichText);
	title_row->addWidget(title, 1);
	walkthrough_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.ShowWalkthrough"));
	walkthrough_btn->setToolTip(tr("IRLSAFETYPlus.Dock.ShowWalkthrough.Tooltip"));
	title_row->addWidget(walkthrough_btn);
	outer->addLayout(title_row);

	privacy_label = new QLabel(tr("IRLSAFETYPlus.Dock.PrivacyBadge"));
	privacy_label->setWordWrap(true);
	privacy_label->setStyleSheet(QStringLiteral("color: #3ecf8e; font-weight: 600;"));
	privacy_label->setToolTip(tr("IRLSAFETYPlus.Dock.PrivacyBadge.Tooltip"));
	outer->addWidget(privacy_label);

	region_label = new QLabel(tr("IRLSAFETYPlus.Dock.RegionNote"));
	region_label->setWordWrap(true);
	region_label->setStyleSheet(QStringLiteral("color: #8ab4f8; font-size: 11px;"));
	outer->addWidget(region_label);

	auto *scroll = new QScrollArea();
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll, 1);

	auto *content = new QWidget();
	scroll->setWidget(content);
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);

	auto *filter_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.FilterGroup"));
	auto *filter_layout = new QVBoxLayout(filter_group);
	filter_combo = new QComboBox();
	filter_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	filter_layout->addWidget(filter_combo);
	layout->addWidget(filter_group);

	auto *status_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.StatusGroup"));
	auto *status_layout = new QVBoxLayout(status_group);
	ocr_status = new QLabel(tr("IRLSAFETYPlus.Dock.OcrChecking"));
	model_status = new QLabel(tr("IRLSAFETYPlus.Dock.ModelChecking"));
	overlay_status = new QLabel(tr("IRLSAFETYPlus.Dock.OverlayIdle"));
	ocr_status->setWordWrap(true);
	model_status->setWordWrap(true);
	overlay_status->setWordWrap(true);
	status_layout->addWidget(ocr_status);
	status_layout->addWidget(model_status);
	status_layout->addWidget(overlay_status);
	layout->addWidget(status_group);

	auto *toggle_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.QuickToggles"));
	auto *toggle_layout = new QVBoxLayout(toggle_group);
	enable_all = new QCheckBox(tr("IRLSAFETYPlus.EnableAll"));
	enable_all->setToolTip(tr("IRLSAFETYPlus.EnableAll.Tooltip"));
	cat_plates = new QCheckBox(tr("IRLSAFETYPlus.CatLicensePlates"));
	cat_plates->setToolTip(tr("IRLSAFETYPlus.CatLicensePlates.Tooltip"));
	cat_signs = new QCheckBox(tr("IRLSAFETYPlus.CatStreetSigns"));
	cat_signs->setToolTip(tr("IRLSAFETYPlus.CatStreetSigns.Tooltip"));
	cat_screen = new QCheckBox(tr("IRLSAFETYPlus.CatScreenText"));
	cat_screen->setToolTip(tr("IRLSAFETYPlus.CatScreenText.Tooltip"));
	toggle_layout->addWidget(enable_all);
	toggle_layout->addWidget(cat_plates);
	toggle_layout->addWidget(cat_signs);
	toggle_layout->addWidget(cat_screen);
	layout->addWidget(toggle_group);

	auto *model_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.ModelGroup"));
	auto *model_layout = new QVBoxLayout(model_group);
	model_path = new QLineEdit();
	model_path->setReadOnly(true);
	model_path->setPlaceholderText(tr("IRLSAFETYPlus.Dock.ModelPathPlaceholder"));
	model_layout->addWidget(model_path);

	auto *model_btn_row = new QHBoxLayout();
	reload_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.ReloadModel"));
	reload_btn->setToolTip(tr("IRLSAFETYPlus.Dock.ReloadModel.Tooltip"));
	auto *browse_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.BrowseModel"));
	model_btn_row->addWidget(reload_btn);
	model_btn_row->addWidget(browse_btn);
	model_layout->addLayout(model_btn_row);

	auto *folder_btn_row = new QHBoxLayout();
	auto *models_folder_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenModelsFolder"));
	auto *training_folder_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenTrainingFolder"));
	folder_btn_row->addWidget(models_folder_btn);
	folder_btn_row->addWidget(training_folder_btn);
	model_layout->addLayout(folder_btn_row);
	layout->addWidget(model_group);

	auto *train_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.TrainingGroup"));
	auto *train_layout = new QVBoxLayout(train_group);
	auto *train_intro = new QLabel(tr("IRLSAFETYPlus.Dock.TrainingIntro"));
	train_intro->setWordWrap(true);
	train_layout->addWidget(train_intro);

	capture_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.CaptureFrame"));
	capture_btn->setToolTip(tr("IRLSAFETYPlus.Dock.CaptureFrame.Tooltip"));
	train_layout->addWidget(capture_btn);
	capture_hint = new QLabel(tr("IRLSAFETYPlus.Dock.CaptureHint"));
	capture_hint->setWordWrap(true);
	capture_hint->setStyleSheet(QStringLiteral("color: #aaaaaa; font-size: 11px;"));
	train_layout->addWidget(capture_hint);

	auto *guide_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenTrainingGuide"));
	train_layout->addWidget(guide_btn);
	layout->addWidget(train_group);

	layout->addStretch(1);

	connect(filter_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&IRLSafetyControlDock::onFilterChanged);
	connect(enable_all, &QCheckBox::toggled, this, &IRLSafetyControlDock::onEnableAllToggled);
	connect(cat_plates, &QCheckBox::toggled, this, &IRLSafetyControlDock::onCategoryToggled);
	connect(cat_signs, &QCheckBox::toggled, this, &IRLSafetyControlDock::onCategoryToggled);
	connect(cat_screen, &QCheckBox::toggled, this, &IRLSafetyControlDock::onCategoryToggled);
	connect(reload_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onReloadModel);
	connect(browse_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onBrowseModel);
	connect(models_folder_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onOpenModelsFolder);
	connect(training_folder_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onOpenTrainingFolder);
	connect(guide_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onOpenTrainingGuide);
	connect(capture_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onCaptureTrainingFrame);
	connect(walkthrough_btn, &QPushButton::clicked, this, &IRLSafetyControlDock::onShowWalkthrough);

	connect(&refresh_timer, &QTimer::timeout, this, &IRLSafetyControlDock::refreshUi);
	refresh_timer.start(750);

	rebuildFilterList();
	refreshUi();
}

IRLSafetyControlDock::~IRLSafetyControlDock()
{
	irlsafety_control_refresh_filters(&filters);
}

obs_source_t *IRLSafetyControlDock::currentFilter() const
{
	if (selected_filter_index < 0 || (size_t)selected_filter_index >= filters.count)
		return nullptr;
	return filters.entries[selected_filter_index].source;
}

void IRLSafetyControlDock::rebuildFilterList()
{
	obs_source_t *previous = currentFilter();

	filter_combo->blockSignals(true);
	filter_combo->clear();

	if (filters.count == 0) {
		filter_combo->addItem(tr("IRLSAFETYPlus.Dock.NoFilters"));
		selected_filter_index = -1;
	} else {
		int restore = 0;
		for (size_t i = 0; i < filters.count; i++) {
			filter_combo->addItem(QString::fromUtf8(filters.entries[i].display_name));
			if (previous && filters.entries[i].source == previous)
				restore = (int)i;
		}
		selected_filter_index = restore;
		filter_combo->setCurrentIndex(selected_filter_index);
	}

	filter_combo->blockSignals(false);
}

void IRLSafetyControlDock::syncTogglesFromStatus()
{
	irlsafety_runtime_status status;
	memset(&status, 0, sizeof(status));
	obs_source_t *filter = currentFilter();

	if (!filter || irlsafety_control_get_status(filter, &status) != 0) {
		enable_all->setEnabled(false);
		cat_plates->setEnabled(false);
		cat_signs->setEnabled(false);
		cat_screen->setEnabled(false);
		return;
	}

	enable_all->setEnabled(true);
	cat_plates->setEnabled(true);
	cat_signs->setEnabled(true);
	cat_screen->setEnabled(true);

	const QSignalBlocker b1(enable_all);
	const QSignalBlocker b2(cat_plates);
	const QSignalBlocker b3(cat_signs);
	const QSignalBlocker b4(cat_screen);
	enable_all->setChecked(status.protection_enabled);
	cat_plates->setChecked(status.cat_license_plates);
	cat_signs->setChecked(status.cat_street_signs);
	cat_screen->setChecked(status.cat_screen_text);
}

void IRLSafetyControlDock::refreshUi()
{
	irlsafety_runtime_status status;
	memset(&status, 0, sizeof(status));
	obs_source_t *filter = currentFilter();
	size_t previous_count = filters.count;

	irlsafety_control_refresh_filters(&filters);
	if (filters.count != previous_count || filter_combo->count() != (int)filters.count)
		rebuildFilterList();

	filter = currentFilter();

	if (filters.count == 0) {
		ocr_status->setText(tr("IRLSAFETYPlus.Dock.NoFiltersHint"));
		model_status->clear();
		overlay_status->clear();
		model_path->clear();
		reload_btn->setEnabled(false);
		capture_btn->setEnabled(false);
		return;
	}

	reload_btn->setEnabled(filter != nullptr);
	capture_btn->setEnabled(true);
	syncTogglesFromStatus();

	if (!filter || irlsafety_control_get_status(filter, &status) != 0)
		return;

	if (status.ocr_available) {
		const char *busy = status.ocr_busy ? "IRLSAFETYPlus.Dock.OcrBusy" : "IRLSAFETYPlus.Dock.OcrReady";
		ocr_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
					    .arg(statusColor(true), tr(busy)));
	} else {
		ocr_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
					    .arg(statusColor(false), tr("IRLSAFETYPlus.Dock.OcrUnavailable")));
	}
	ocr_status->setTextFormat(Qt::RichText);

	if (status.detector_ready) {
		model_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
					      .arg(statusColor(true), tr("IRLSAFETYPlus.Dock.ModelReady")));
	} else if (status.model_file_exists) {
		model_status->setText(QStringLiteral("<span style='color:%1'>●</span> %3<br><span style='color:#ccc'>%2</span>")
					      .arg(statusColor(false), QString::fromUtf8(status.detector_message),
						   tr("IRLSAFETYPlus.Dock.ModelNotLoaded")));
	} else {
		model_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
					      .arg(statusColor(false), tr("IRLSAFETYPlus.Dock.ModelMissing")));
	}
	model_status->setTextFormat(Qt::RichText);

	overlay_status->setText(tr("IRLSAFETYPlus.Dock.OverlayCount").arg(status.overlay_count));
	if (status.protection_enabled)
		model_path->setText(QString::fromUtf8(status.model_path));
	else
		model_path->setText(tr("IRLSAFETYPlus.Dock.ProtectionOff"));
}

void IRLSafetyControlDock::onFilterChanged(int index)
{
	selected_filter_index = index;
	syncTogglesFromStatus();
	refreshUi();
}

void IRLSafetyControlDock::onEnableAllToggled(bool checked)
{
	obs_source_t *filter = currentFilter();
	if (filter)
		irlsafety_control_set_bool_setting(filter, IRLSAFETY_SET_ENABLE_ALL, checked);
}

void IRLSafetyControlDock::onCategoryToggled(bool checked)
{
	QCheckBox *box = qobject_cast<QCheckBox *>(sender());
	obs_source_t *filter = currentFilter();
	const char *key = nullptr;

	if (!box || !filter)
		return;

	if (box == cat_plates)
		key = IRLSAFETY_SET_CAT_LICENSE_PLATES;
	else if (box == cat_signs)
		key = IRLSAFETY_SET_CAT_STREET_SIGNS;
	else if (box == cat_screen)
		key = IRLSAFETY_SET_CAT_SCREEN_TEXT;

	if (key)
		irlsafety_control_set_bool_setting(filter, key, checked);
}

void IRLSafetyControlDock::onReloadModel()
{
	obs_source_t *filter = currentFilter();
	if (!filter)
		return;

	if (irlsafety_control_reload_model(filter) == 0)
		QMessageBox::information(this, tr("IRLSAFETYPlus.Dock.ReloadModel"),
					 tr("IRLSAFETYPlus.Dock.ReloadOk"));
	else
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.ReloadModel"),
				     tr("IRLSAFETYPlus.Dock.ReloadFailed"));
}

void IRLSafetyControlDock::onBrowseModel()
{
	obs_source_t *filter = currentFilter();
	QString path = QFileDialog::getOpenFileName(this, tr("IRLSAFETYPlus.Dock.BrowseModel"), QString(),
						    QStringLiteral("ONNX models (*.onnx)"));
	if (path.isEmpty() || !filter)
		return;

	if (irlsafety_control_set_model_path(filter, path.toUtf8().constData()) == 0)
		refreshUi();
}

void IRLSafetyControlDock::onOpenModelsFolder()
{
	char folder[1024];
	irlsafety_control_get_models_folder(folder, sizeof(folder));
	if (folder[0] != '\0')
		irlsafety_control_open_path(folder);
}

void IRLSafetyControlDock::onOpenTrainingFolder()
{
	char folder[1024];
	irlsafety_control_get_training_folder(folder, sizeof(folder));
	if (folder[0] == '\0')
		return;

	irlsafety_control_ensure_dir(folder);
	char train_images[1200];
	snprintf(train_images, sizeof(train_images), "%s/images/train", folder);
	irlsafety_control_ensure_dir(train_images);
	irlsafety_control_open_path(train_images);
}

void IRLSafetyControlDock::onOpenTrainingGuide()
{
	char *guide = obs_module_file("models/TRAINING.txt");
	if (guide) {
		irlsafety_control_open_path(guide);
		bfree(guide);
	}
}

void IRLSafetyControlDock::onShowWalkthrough()
{
	irlsafety_onboarding_show();
}

void IRLSafetyControlDock::onCaptureTrainingFrame()
{
	if (irlsafety_control_save_training_screenshot() == 0) {
		capture_hint->setText(tr("IRLSAFETYPlus.Dock.CaptureSaved"));
		onOpenTrainingFolder();
	} else {
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.CaptureFrame"),
				     tr("IRLSAFETYPlus.Dock.CaptureFailed"));
	}
}

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
	delete g_control_dock;
	g_control_dock = nullptr;
}