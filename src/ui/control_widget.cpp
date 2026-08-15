/*
 * IRLSAFETY+ — shared control panel widget (dock + tray).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "control_widget.hpp"
#include "censor_log_dialog.hpp"
#include "onboarding_dialog.hpp"

#include "../irlsafety_control.h"
#include "../filter_settings.h"
#include "../irlsafety_shutdown.h"
#include "../virtual_cam/virtual_cam.h"

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

static QString tr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QString statusColor(bool ok)
{
	return ok ? QStringLiteral("#3ecf8e") : QStringLiteral("#f0a030");
}

static QString loadTierLabel(uint8_t tier)
{
	switch (tier) {
	case 2:
		return tr("IRLSAFETYPlus.Dock.LoadHigh");
	case 1:
		return tr("IRLSAFETYPlus.Dock.LoadMedium");
	default:
		return tr("IRLSAFETYPlus.Dock.LoadLow");
	}
}

IRLSafetyControlWidget::IRLSafetyControlWidget(QWidget *parent, bool compact) : QWidget(parent), compact_(compact)
{
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(compact_ ? 6 : 8, compact_ ? 6 : 8, compact_ ? 6 : 8, compact_ ? 6 : 8);
	outer->setSpacing(compact_ ? 4 : 6);

	if (!compact_) {
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
	} else {
		walkthrough_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.ShowWalkthrough"));
		walkthrough_btn->setToolTip(tr("IRLSAFETYPlus.Dock.ShowWalkthrough.Tooltip"));
		walkthrough_btn->hide();
	}

	auto *scroll = new QScrollArea();
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll, 1);

	support_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.SupportDev"));
	support_btn->setToolTip(tr("IRLSAFETYPlus.Dock.SupportDev.Tooltip"));
	support_btn->setFlat(true);
	support_btn->setStyleSheet(QStringLiteral("color: #8ab4f8; font-size: 11px; text-align: left;"));
	support_btn->setCursor(Qt::PointingHandCursor);
	outer->addWidget(support_btn);

	auto *content = new QWidget();
	scroll->setWidget(content);
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);

	auto *filter_group = new QGroupBox(compact_ ? QString() : tr("IRLSAFETYPlus.Dock.FilterGroup"));
	if (compact_)
		filter_group->setFlat(true);
	auto *filter_layout = new QVBoxLayout(filter_group);
	filter_combo = new QComboBox();
	filter_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	filter_layout->addWidget(filter_combo);
	layout->addWidget(filter_group);

	resource_metrics = new QLabel(tr("IRLSAFETYPlus.Dock.ResourceIdle"));
	resource_metrics->setWordWrap(true);
	resource_metrics->setStyleSheet(QStringLiteral("color: #9aa0a6; font-size: 11px;"));
	layout->addWidget(resource_metrics);

	status_group = new QGroupBox(compact_ ? QString() : tr("IRLSAFETYPlus.Dock.StatusGroup"));
	if (compact_)
		status_group->setFlat(true);
	auto *status_layout = new QVBoxLayout(status_group);
	ocr_status = new QLabel(tr("IRLSAFETYPlus.Dock.OcrChecking"));
	model_status = new QLabel(tr("IRLSAFETYPlus.Dock.ModelChecking"));
	overlay_status = new QLabel(tr("IRLSAFETYPlus.Dock.OverlayIdle"));
	vcam_status = new QLabel();
	ocr_status->setWordWrap(true);
	model_status->setWordWrap(true);
	overlay_status->setWordWrap(true);
	vcam_status->setWordWrap(true);
	vcam_status->setStyleSheet(QStringLiteral("color: #aaaaaa; font-size: 11px;"));
	status_layout->addWidget(ocr_status);
	status_layout->addWidget(model_status);
	status_layout->addWidget(overlay_status);
	status_layout->addWidget(vcam_status);

	auto *vcam_btn_row = new QHBoxLayout();
	vcam_start_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.StartVirtualCam"));
	vcam_start_btn->setToolTip(tr("IRLSAFETYPlus.Dock.StartVirtualCam.Tooltip"));
	vcam_stop_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.StopVirtualCam"));
	vcam_stop_btn->setToolTip(tr("IRLSAFETYPlus.Dock.StopVirtualCam.Tooltip"));
	vcam_btn_row->addWidget(vcam_start_btn);
	vcam_btn_row->addWidget(vcam_stop_btn);
	status_layout->addLayout(vcam_btn_row);

	censor_log_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.CensorLog"));
	censor_log_btn->setToolTip(tr("IRLSAFETYPlus.Dock.CensorLog.Tooltip"));
	status_layout->addWidget(censor_log_btn);
	layout->addWidget(status_group);

	auto *toggle_group = new QGroupBox(compact_ ? tr("IRLSAFETYPlus.Dock.QuickTogglesShort") : tr("IRLSAFETYPlus.Dock.QuickToggles"));
	auto *toggle_layout = new QVBoxLayout(toggle_group);
	enable_all = new QCheckBox(tr("IRLSAFETYPlus.EnableAll"));
	enable_all->setToolTip(tr("IRLSAFETYPlus.EnableAll.Tooltip"));
	cat_plates = new QCheckBox(tr("IRLSAFETYPlus.CatLicensePlates"));
	cat_plates->setToolTip(tr("IRLSAFETYPlus.CatLicensePlates.Tooltip"));
	cat_signs = new QCheckBox(tr("IRLSAFETYPlus.CatStreetSigns"));
	cat_signs->setToolTip(tr("IRLSAFETYPlus.CatStreetSigns.Tooltip"));
	cat_mail = new QCheckBox(tr("IRLSAFETYPlus.CatShippingLabels"));
	cat_mail->setToolTip(tr("IRLSAFETYPlus.CatShippingLabels.Tooltip"));
	cat_ids = new QCheckBox(tr("IRLSAFETYPlus.CatIdDocuments"));
	cat_ids->setToolTip(tr("IRLSAFETYPlus.CatIdDocuments.Tooltip"));
	cat_screen = new QCheckBox(tr("IRLSAFETYPlus.CatScreenText"));
	cat_screen->setToolTip(tr("IRLSAFETYPlus.CatScreenText.Tooltip"));
	toggle_layout->addWidget(enable_all);
	toggle_layout->addWidget(cat_plates);
	toggle_layout->addWidget(cat_signs);
	toggle_layout->addWidget(cat_mail);
	toggle_layout->addWidget(cat_ids);
	toggle_layout->addWidget(cat_screen);
	layout->addWidget(toggle_group);

	model_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.ModelGroup"));
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

	training_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.TrainingGroup"));
	auto *train_layout = new QVBoxLayout(training_group);
	auto *train_intro = new QLabel(tr("IRLSAFETYPlus.Dock.TrainingIntro"));
	train_intro->setWordWrap(true);
	train_layout->addWidget(train_intro);

	capture_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.CaptureFrame"));
	capture_btn->setToolTip(tr("IRLSAFETYPlus.Dock.CaptureFrame.Tooltip"));
	train_layout->addWidget(capture_btn);

	auto *train_row = new QHBoxLayout();
	label_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.LabelImages"));
	label_btn->setToolTip(tr("IRLSAFETYPlus.Dock.LabelImages.Tooltip"));
	train_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.TrainModel"));
	train_btn->setToolTip(tr("IRLSAFETYPlus.Dock.TrainModel.Tooltip"));
	train_row->addWidget(label_btn);
	train_row->addWidget(train_btn);
	train_layout->addLayout(train_row);
	capture_hint = new QLabel(tr("IRLSAFETYPlus.Dock.CaptureHint"));
	capture_hint->setWordWrap(true);
	capture_hint->setStyleSheet(QStringLiteral("color: #aaaaaa; font-size: 11px;"));
	train_layout->addWidget(capture_hint);

	auto *guide_row = new QHBoxLayout();
	auto *guide_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenTrainingGuide"));
	auto *session_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenTrainingSession"));
	session_btn->setToolTip(tr("IRLSAFETYPlus.Dock.OpenTrainingSession.Tooltip"));
	guide_row->addWidget(guide_btn);
	guide_row->addWidget(session_btn);
	train_layout->addLayout(guide_row);
	layout->addWidget(training_group);

	resource_group = new QGroupBox(tr("IRLSAFETYPlus.Dock.ResourcesGroup"));
	auto *resource_layout = new QVBoxLayout(resource_group);
	auto *resource_intro = new QLabel(tr("IRLSAFETYPlus.Dock.ResourcesIntro"));
	resource_intro->setWordWrap(true);
	resource_layout->addWidget(resource_intro);
	auto *resource_row = new QHBoxLayout();
	auto *ocr_guide_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenOcrGuide"));
	ocr_guide_btn->setToolTip(tr("IRLSAFETYPlus.Dock.OpenOcrGuide.Tooltip"));
	auto *platforms_guide_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenPlatformsGuide"));
	platforms_guide_btn->setToolTip(tr("IRLSAFETYPlus.Dock.OpenPlatformsGuide.Tooltip"));
	auto *vcam_guide_btn = new QPushButton(tr("IRLSAFETYPlus.Dock.OpenVirtualCamGuide"));
	vcam_guide_btn->setToolTip(tr("IRLSAFETYPlus.Dock.OpenVirtualCamGuide.Tooltip"));
	resource_row->addWidget(ocr_guide_btn);
	resource_row->addWidget(platforms_guide_btn);
	resource_row->addWidget(vcam_guide_btn);
	resource_layout->addLayout(resource_row);
	layout->addWidget(resource_group);

	layout->addStretch(1);

	connect(filter_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&IRLSafetyControlWidget::onFilterChanged);
	connect(enable_all, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onEnableAllToggled);
	connect(cat_plates, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onCategoryToggled);
	connect(cat_signs, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onCategoryToggled);
	connect(cat_mail, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onCategoryToggled);
	connect(cat_ids, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onCategoryToggled);
	connect(cat_screen, &QCheckBox::toggled, this, &IRLSafetyControlWidget::onCategoryToggled);
	connect(reload_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onReloadModel);
	connect(browse_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onBrowseModel);
	connect(models_folder_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenModelsFolder);
	connect(training_folder_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenTrainingFolder);
	connect(guide_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenTrainingGuide);
	connect(session_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenTrainingSession);
	connect(ocr_guide_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenOcrGuide);
	connect(platforms_guide_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenPlatformsGuide);
	connect(vcam_guide_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onOpenVirtualCamGuide);
	connect(vcam_start_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onStartVirtualCam);
	connect(vcam_stop_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onStopVirtualCam);
	connect(capture_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onCaptureTrainingFrame);
	connect(label_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onLabelImages);
	connect(train_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onTrainModel);
	if (walkthrough_btn)
		connect(walkthrough_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onShowWalkthrough);
	connect(support_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onSupportDevelopment);
	connect(censor_log_btn, &QPushButton::clicked, this, &IRLSafetyControlWidget::onShowCensorLog);

	connect(&refresh_timer, &QTimer::timeout, this, &IRLSafetyControlWidget::refreshUi);

	applyCompactChrome();
	if (!compact_) {
		refresh_timer.start(750);
		rebuildFilterList();
		refreshUi();
	}
}

void IRLSafetyControlWidget::startPeriodicRefresh()
{
	if (refresh_timer.isActive())
		return;

	refresh_timer.start(750);
	rebuildFilterList();
	refreshUi();
}

void IRLSafetyControlWidget::applyCompactChrome()
{
	if (!compact_)
		return;

	setObjectName(QStringLiteral("irlsafetyCompactRoot"));
	if (training_group)
		training_group->hide();
	if (resource_group)
		resource_group->hide();
	if (model_group)
		model_group->hide();
	if (privacy_label)
		privacy_label->hide();
	if (region_label)
		region_label->hide();
	if (support_btn)
		support_btn->hide();
	if (ocr_status)
		ocr_status->hide();
	if (model_status)
		model_status->hide();
	if (overlay_status)
		overlay_status->hide();
	if (vcam_status)
		vcam_status->hide();
	if (vcam_start_btn && vcam_stop_btn) {
		vcam_start_btn->hide();
		vcam_stop_btn->hide();
	}
}

QString IRLSafetyControlWidget::formatResourceLine(const irlsafety_runtime_status &status) const
{
	const QString tier = loadTierLabel(status.load_tier);
	const QString ep = QString::fromUtf8(status.detector_ep[0] ? status.detector_ep : "CPU");

	if (!status.protection_enabled)
		return tr("IRLSAFETYPlus.Dock.ResourceOff");

	if (status.detector_ready) {
		return tr("IRLSAFETYPlus.Dock.ResourceLine")
			.arg(status.last_yolo_ms, 0, 'f', 1)
			.arg(ep)
			.arg(status.approx_scans_per_min)
			.arg(status.overlay_count)
			.arg(tier);
	}

	if (status.model_file_exists)
		return tr("IRLSAFETYPlus.Dock.ResourceModelFound").arg(status.approx_scans_per_min).arg(tier);

	return tr("IRLSAFETYPlus.Dock.ResourceNoModel").arg(status.approx_scans_per_min).arg(tier);
}

IRLSafetyControlWidget::~IRLSafetyControlWidget()
{
	refresh_timer.stop();
	irlsafety_control_release_filters(&filters);
}

obs_source_t *IRLSafetyControlWidget::currentFilter() const
{
	if (selected_filter_index < 0 || (size_t)selected_filter_index >= filters.count)
		return nullptr;
	return filters.entries[selected_filter_index].source;
}

void IRLSafetyControlWidget::rebuildFilterList()
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

void IRLSafetyControlWidget::syncTogglesFromStatus()
{
	irlsafety_runtime_status status;
	memset(&status, 0, sizeof(status));
	obs_source_t *filter = currentFilter();

	if (!filter || irlsafety_control_get_status(filter, &status) != 0) {
		enable_all->setEnabled(false);
		cat_plates->setEnabled(false);
		cat_signs->setEnabled(false);
		cat_mail->setEnabled(false);
		cat_ids->setEnabled(false);
		cat_screen->setEnabled(false);
		return;
	}

	enable_all->setEnabled(true);
	cat_plates->setEnabled(true);
	cat_signs->setEnabled(true);
	cat_mail->setEnabled(true);
	cat_ids->setEnabled(true);
	cat_screen->setEnabled(true);

	const QSignalBlocker b1(enable_all);
	const QSignalBlocker b2(cat_plates);
	const QSignalBlocker b3(cat_signs);
	const QSignalBlocker b4(cat_mail);
	const QSignalBlocker b5(cat_ids);
	const QSignalBlocker b6(cat_screen);
	enable_all->setChecked(status.protection_enabled);
	cat_plates->setChecked(status.cat_license_plates);
	cat_signs->setChecked(status.cat_street_signs);
	cat_mail->setChecked(status.cat_shipping_labels);
	cat_ids->setChecked(status.cat_id_documents);
	cat_screen->setChecked(status.cat_screen_text);
}

void IRLSafetyControlWidget::refreshUi()
{
	irlsafety_runtime_status status;
	memset(&status, 0, sizeof(status));
	obs_source_t *filter = currentFilter();
	size_t previous_count = filters.count;

	if (irlsafety_is_shutting_down())
		return;

	irlsafety_control_refresh_filters(&filters);
	if (filters.count != previous_count || filter_combo->count() != (int)filters.count)
		rebuildFilterList();

	filter = currentFilter();

	if (filters.count == 0) {
		if (resource_metrics)
			resource_metrics->setText(tr("IRLSAFETYPlus.Dock.NoFiltersHint"));
		if (ocr_status)
			ocr_status->setText(tr("IRLSAFETYPlus.Dock.NoFiltersHint"));
		if (model_status)
			model_status->clear();
		if (overlay_status)
			overlay_status->clear();
		if (model_path)
			model_path->clear();
		if (reload_btn)
			reload_btn->setEnabled(false);
		if (capture_btn)
			capture_btn->setEnabled(false);
		if (censor_log_btn)
			censor_log_btn->setEnabled(false);
		return;
	}

	if (reload_btn)
		reload_btn->setEnabled(filter != nullptr);
	if (capture_btn)
		capture_btn->setEnabled(true);
	if (censor_log_btn)
		censor_log_btn->setEnabled(filter != nullptr);
	syncTogglesFromStatus();

	if (!filter || irlsafety_control_get_status(filter, &status) != 0)
		return;

	if (resource_metrics)
		resource_metrics->setText(formatResourceLine(status));

	if (ocr_status) {
		if (status.ocr_available) {
			const char *busy = status.ocr_busy ? "IRLSAFETYPlus.Dock.OcrBusy" : "IRLSAFETYPlus.Dock.OcrReady";
			ocr_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2<br><span style='color:#aaa;font-size:11px'>%3</span>")
						    .arg(statusColor(true), tr(busy), QString::fromUtf8(status.ocr_backend)));
		} else {
			ocr_status->setText(
				QStringLiteral("<span style='color:%1'>●</span> %2<br><span style='color:#aaa;font-size:11px'>%3</span>")
					.arg(statusColor(false), tr("IRLSAFETYPlus.Dock.OcrUnavailable"),
					     QString::fromUtf8(status.ocr_backend[0] ? status.ocr_backend
										       : status.ocr_backend_status)));
		}
		ocr_status->setTextFormat(Qt::RichText);

		if (model_status) {
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
		}

		if (overlay_status)
			overlay_status->setText(tr("IRLSAFETYPlus.Dock.OverlayCount").arg(status.overlay_count));

		const bool vcam_supported = irlsafety_virtual_cam_supported();
		const bool vcam_running = irlsafety_virtual_cam_get_state() == IRLSAFETY_VCAM_RUNNING;
		if (vcam_status) {
			if (vcam_supported) {
				const char *dot_color = vcam_running ? "#3ecf8e" : "#8ab4f8";
				vcam_status->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
							     .arg(dot_color, QString::fromUtf8(irlsafety_virtual_cam_status_message())));
			} else {
				vcam_status->setText(tr("IRLSAFETYPlus.Dock.VirtualCamPlanned"));
			}
			vcam_status->setTextFormat(Qt::RichText);
		}
		if (vcam_start_btn && vcam_stop_btn) {
			vcam_start_btn->setEnabled(vcam_supported && !vcam_running);
			vcam_stop_btn->setEnabled(vcam_supported && vcam_running);
		}
	}

	if (model_path) {
		if (status.protection_enabled)
			model_path->setText(QString::fromUtf8(status.model_path));
		else
			model_path->setText(tr("IRLSAFETYPlus.Dock.ProtectionOff"));
	}
}

void IRLSafetyControlWidget::onFilterChanged(int index)
{
	selected_filter_index = index;
	syncTogglesFromStatus();
	refreshUi();
}

void IRLSafetyControlWidget::onEnableAllToggled(bool checked)
{
	obs_source_t *filter = currentFilter();
	if (filter)
		irlsafety_control_set_bool_setting(filter, IRLSAFETY_SET_ENABLE_ALL, checked);
}

void IRLSafetyControlWidget::onCategoryToggled(bool checked)
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
	else if (box == cat_mail)
		key = IRLSAFETY_SET_CAT_SHIPPING_LABELS;
	else if (box == cat_ids)
		key = IRLSAFETY_SET_CAT_ID_DOCUMENTS;
	else if (box == cat_screen)
		key = IRLSAFETY_SET_CAT_SCREEN_TEXT;

	if (key)
		irlsafety_control_set_bool_setting(filter, key, checked);
}

void IRLSafetyControlWidget::onReloadModel()
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

void IRLSafetyControlWidget::onBrowseModel()
{
	obs_source_t *filter = currentFilter();
	QString path = QFileDialog::getOpenFileName(this, tr("IRLSAFETYPlus.Dock.BrowseModel"), QString(),
						    QStringLiteral("ONNX models (*.onnx)"));
	if (path.isEmpty() || !filter)
		return;

	if (irlsafety_control_set_model_path(filter, path.toUtf8().constData()) == 0)
		refreshUi();
}

void IRLSafetyControlWidget::onOpenModelsFolder()
{
	char folder[1024];
	irlsafety_control_get_models_folder(folder, sizeof(folder));
	if (folder[0] != '\0')
		irlsafety_control_open_path(folder);
}

void IRLSafetyControlWidget::onOpenTrainingFolder()
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

void IRLSafetyControlWidget::onOpenTrainingGuide()
{
	irlsafety_control_open_models_guide("TRAINING.txt");
}

void IRLSafetyControlWidget::onOpenTrainingSession()
{
	irlsafety_control_open_models_guide("TRAINING_SESSION.txt");
}

void IRLSafetyControlWidget::onOpenOcrGuide()
{
	irlsafety_control_open_models_guide("OCR.txt");
}

void IRLSafetyControlWidget::onOpenPlatformsGuide()
{
	irlsafety_control_open_models_guide("PLATFORMS.txt");
}

void IRLSafetyControlWidget::onOpenVirtualCamGuide()
{
	irlsafety_control_open_models_guide("VIRTUAL_CAMERA.txt");
}

void IRLSafetyControlWidget::onStartVirtualCam()
{
	irlsafety_virtual_cam_config config = {1920, 1080, 30, true};
	if (irlsafety_virtual_cam_start(&config) != 0)
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.StartVirtualCam"),
				     tr("IRLSAFETYPlus.Dock.VirtualCamStartFailed"));
	refreshUi();
}

void IRLSafetyControlWidget::onStopVirtualCam()
{
	irlsafety_virtual_cam_stop();
	refreshUi();
}

void IRLSafetyControlWidget::onShowWalkthrough()
{
	irlsafety_onboarding_show();
}

void IRLSafetyControlWidget::onShowCensorLog()
{
	irlsafety_censor_log_show(this, currentFilter());
}

void IRLSafetyControlWidget::onLabelImages()
{
	if (irlsafety_control_run_script("scripts/label-images.ps1") != 0)
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.LabelImages"),
				     tr("IRLSAFETYPlus.Dock.TrainScriptFailed"));
}

void IRLSafetyControlWidget::onTrainModel()
{
	if (irlsafety_control_run_script("scripts/train-model.ps1") != 0)
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.TrainModel"),
				     tr("IRLSAFETYPlus.Dock.TrainScriptFailed"));
}

void IRLSafetyControlWidget::onSupportDevelopment()
{
	irlsafety_control_open_path(
		"https://github.com/sponsors/medicinalsheep?frequency=one-time&sponsor=medicinalsheep");
}

void IRLSafetyControlWidget::onCaptureTrainingFrame()
{
	if (irlsafety_control_save_training_screenshot() == 0) {
		capture_hint->setText(tr("IRLSAFETYPlus.Dock.CaptureSaved"));
		onOpenTrainingFolder();
	} else {
		QMessageBox::warning(this, tr("IRLSAFETYPlus.Dock.CaptureFrame"),
				     tr("IRLSAFETYPlus.Dock.CaptureFailed"));
	}
}

