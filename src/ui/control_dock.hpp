/*
 * IRLSAFETY+ — OBS dock shell for IRLSAFETY+ control widget.
 * Copyright (c) 2026 medicinalsheep. MIT License.
 */

#pragma once

#include <QFrame>

class IRLSafetyControlDock : public QFrame {
	Q_OBJECT

public:
	explicit IRLSafetyControlDock(QWidget *parent = nullptr);
	~IRLSafetyControlDock() override;
};

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_control_dock_register(void);
void irlsafety_control_dock_unregister(void);

#ifdef __cplusplus
}
#endif