/*
 * IRLSAFETY+ — OBS control dock (model + training hub).
 * Copyright (c) 2026 medicinalsheep. MIT License.
 */

#pragma once

#include "../irlsafety_control.h"

#include <QFrame>
#include <QTimer>

struct obs_source;

class QLabel;
class QComboBox;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QGroupBox;

class IRLSafetyControlDock : public QFrame {
	Q_OBJECT

public:
	explicit IRLSafetyControlDock(QWidget *parent = nullptr);
	~IRLSafetyControlDock() override;

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
	void onOpenOcrGuide();
	void onOpenPlatformsGuide();
	void onOpenVirtualCamGuide();
	void onCaptureTrainingFrame();
	void onShowWalkthrough();
	void onLabelImages();
	void onTrainModel();
	void onSupportDevelopment();

private:
	void rebuildFilterList();
	void syncTogglesFromStatus();
	struct obs_source *currentFilter() const;

	QTimer refresh_timer;
	irlsafety_filter_list filters {};
	int selected_filter_index = -1;

	QLabel *privacy_label = nullptr;
	QLabel *region_label = nullptr;
	QComboBox *filter_combo = nullptr;
	QLabel *ocr_status = nullptr;
	QLabel *model_status = nullptr;
	QLabel *overlay_status = nullptr;
	QLabel *vcam_status = nullptr;
	QLineEdit *model_path = nullptr;
	QCheckBox *enable_all = nullptr;
	QCheckBox *cat_plates = nullptr;
	QCheckBox *cat_signs = nullptr;
	QCheckBox *cat_screen = nullptr;
	QCheckBox *cat_sensitive = nullptr;
	QPushButton *reload_btn = nullptr;
	QPushButton *walkthrough_btn = nullptr;
	QPushButton *capture_btn = nullptr;
	QPushButton *label_btn = nullptr;
	QPushButton *train_btn = nullptr;
	QLabel *capture_hint = nullptr;
	QPushButton *support_btn = nullptr;
};

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_control_dock_register(void);
void irlsafety_control_dock_unregister(void);

#ifdef __cplusplus
}
#endif