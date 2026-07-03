/*
 * IRLSAFETY+ — filter property tooltips (OBS long descriptions).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "filter_property_ui.h"

#include "filter_settings.h"

#ifdef IRLSAFETY_TEST_BUILD
#include "obs-mock.h"
#else
#include <obs-module.h>
#endif

static void set_tip(struct obs_property *prop, const char *locale_key)
{
	if (!prop || !locale_key)
		return;
	obs_property_set_long_description(prop, obs_module_text(locale_key));
}

static void set_tip_on(struct obs_properties *props, const char *prop_id, const char *locale_key)
{
	if (!props || !prop_id)
		return;
	set_tip(obs_properties_get(props, prop_id), locale_key);
}

void irlsafety_filter_apply_property_tooltips(struct obs_properties *props, struct obs_properties *categories,
					      struct obs_properties *custom, struct obs_properties *protection,
					      struct obs_properties *advanced)
{
	set_tip_on(props, "info", "IRLSAFETYPlus.FilterDescription.Tooltip");
	set_tip_on(props, IRLSAFETY_SET_ENABLE_ALL, "IRLSAFETYPlus.EnableAll.Tooltip");
	set_tip_on(props, IRLSAFETY_SET_TEST_EFFECT, "IRLSAFETYPlus.TestEffect.Tooltip");
	set_tip_on(props, "categories", "IRLSAFETYPlus.GroupCategories.Tooltip");
	set_tip_on(props, "custom_pii", "IRLSAFETYPlus.GroupCustomPii.Tooltip");
	set_tip_on(props, "protection", "IRLSAFETYPlus.GroupProtection.Tooltip");
	set_tip_on(props, "advanced", "IRLSAFETYPlus.GroupAdvanced.Tooltip");

	set_tip_on(categories, IRLSAFETY_SET_CAT_LICENSE_PLATES, "IRLSAFETYPlus.CatLicensePlates.Tooltip");
	set_tip_on(categories, IRLSAFETY_SET_CAT_STREET_SIGNS, "IRLSAFETYPlus.CatStreetSigns.Tooltip");
	set_tip_on(categories, IRLSAFETY_SET_CAT_SHIPPING_LABELS, "IRLSAFETYPlus.CatShippingLabels.Tooltip");
	set_tip_on(categories, IRLSAFETY_SET_CAT_ID_DOCUMENTS, "IRLSAFETYPlus.CatIdDocuments.Tooltip");
	set_tip_on(categories, IRLSAFETY_SET_CAT_SCREEN_TEXT, "IRLSAFETYPlus.CatScreenText.Tooltip");
	set_tip_on(categories, IRLSAFETY_SET_CAT_CUSTOM_PII, "IRLSAFETYPlus.CatCustomPii.Tooltip");

	set_tip_on(custom, "custom_pii_hint", "IRLSAFETYPlus.CustomPiiHint.Tooltip");
	set_tip_on(custom, IRLSAFETY_SET_CUSTOM_PII_INLINE, "IRLSAFETYPlus.CustomPiiInline.Tooltip");
	set_tip_on(custom, IRLSAFETY_SET_CUSTOM_PII_FILE, "IRLSAFETYPlus.CustomPiiFile.Tooltip");

	set_tip_on(protection, IRLSAFETY_SET_CONFIDENCE, "IRLSAFETYPlus.Confidence.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_FRAME_SKIP, "IRLSAFETYPlus.FrameSkip.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_CENSOR_MODE, "IRLSAFETYPlus.CensorMode.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_CENSOR_OVERLAY, "IRLSAFETYPlus.CensorOverlay.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_CENSOR_COLOR, "IRLSAFETYPlus.CensorColor.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_ANGLED_COVER, "IRLSAFETYPlus.AngledCover.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_HYBRID_DELAY_ENABLE, "IRLSAFETYPlus.HybridDelayEnable.Tooltip");
	set_tip_on(protection, IRLSAFETY_SET_GLOBAL_DELAY_SEC, "IRLSAFETYPlus.GlobalDelaySec.Tooltip");

	set_tip_on(advanced, IRLSAFETY_SET_OCR_DETAIL, "IRLSAFETYPlus.OcrDetail.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_BLUR_STRENGTH, "IRLSAFETYPlus.BlurStrength.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_AUTO_DELAY_SEC, "IRLSAFETYPlus.AutoDelaySec.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC, "IRLSAFETYPlus.AutoDelayHoldSec.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_PARTIAL_PII_THRESHOLD, "IRLSAFETYPlus.PartialPiiThreshold.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_COVER_WHILE_TYPING, "IRLSAFETYPlus.CoverWhileTyping.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_SECURE_MODE_ENABLE, "IRLSAFETYPlus.SecureModeEnable.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_SECURE_DROP_FRAMES, "IRLSAFETYPlus.SecureDropFrames.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_CAT_SENSITIVE_PATTERNS, "IRLSAFETYPlus.CatSensitivePatterns.Tooltip");
	set_tip_on(advanced, "hybrid_delay_audio_note", "IRLSAFETYPlus.HybridDelayAudioNote.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_PREFER_GPU, "IRLSAFETYPlus.PreferGpu.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_USE_CHILD_OCR, "IRLSAFETYPlus.UseChildOcr.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_SHOW_PREVIEW, "IRLSAFETYPlus.ShowPreview.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_ENABLE_LOGGING, "IRLSAFETYPlus.EnableLogging.Tooltip");
	set_tip_on(advanced, IRLSAFETY_SET_MODEL_PATH, "IRLSAFETYPlus.ModelPath.Tooltip");
}