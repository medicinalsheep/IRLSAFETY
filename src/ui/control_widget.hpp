/*
 * IRLSAFETY+ — shared control panel widget (dock + tray).
 * Copyright (c) 2026 medicinalsheep. MIT License.
 */

#pragma once

#include "../irlsafety_control.h"

#include <QTimer>
#include <QWidget>

struct obs_source;

class QLabel;
class QComboBox;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QGroupBox;

class IRLSafetyControlWidget : public QWidget {
	Q_OBJECT

public:
	explicit IRLSafetyControlWidget(QWidget *parent = nullptr, bool compact = false);
	~IRLSafetyControlWidget() override;

private slots:
	void refreshUi();
	void onFilterChanged(int index);
	void onEnableAllToggled(bool checked);
	void onCategoryToggled(bool checked);
	void onReloadModel();
	void onBrowseModel();
	void onOpenModelsFolder();
	void onOpenTrainingFolder();
	void onOpenTrainingGuide();
	void onOpenTrainingSession();
	void onOpenOcrGuide();
	void onOpenPlatformsGuide();
	void onOpenVirtualCamGuide();
	void onStartVirtualCam();
	void onStopVirtualCam();
	void onCaptureTrainingFrame();
	void onShowWalkthrough();
	void onLabelImages();
	void onTrainModel();
	void onSupportDevelopment();
	void onShowCensorLog();

private:
	void rebuildFilterList();
	void syncTogglesFromStatus();
	void applyCompactChrome();
	struct obs_source *currentFilter() const;
	QString formatResourceLine(const irlsafety_runtime_status &status) const;

	bool compact_ = false;
	QTimer refresh_timer;
	irlsafety_filter_list filters {};
	int selected_filter_index = -1;

	QLabel *privacy_label = nullptr;
	QLabel *region_label = nullptr;
	QLabel *resource_metrics = nullptr;
	QComboBox *filter_combo = nullptr;
	QLabel *ocr_status = nullptr;
	QLabel *model_status = nullptr;
	QLabel *overlay_status = nullptr;
	QLabel *vcam_status = nullptr;
	QLineEdit *model_path = nullptr;
	QCheckBox *enable_all = nullptr;
	QCheckBox *cat_plates = nullptr;
	QCheckBox *cat_signs = nullptr;
	QCheckBox *cat_mail = nullptr;
	QCheckBox *cat_ids = nullptr;
	QCheckBox *cat_screen = nullptr;
	QPushButton *reload_btn = nullptr;
	QPushButton *walkthrough_btn = nullptr;
	QPushButton *capture_btn = nullptr;
	QPushButton *label_btn = nullptr;
	QPushButton *train_btn = nullptr;
	QLabel *capture_hint = nullptr;
	QPushButton *vcam_start_btn = nullptr;
	QPushButton *vcam_stop_btn = nullptr;
	QPushButton *censor_log_btn = nullptr;
	QPushButton *support_btn = nullptr;
	QGroupBox *training_group = nullptr;
	QGroupBox *resource_group = nullptr;
	QGroupBox *model_group = nullptr;
	QGroupBox *status_group = nullptr;
};