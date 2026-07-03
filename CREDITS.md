# IRLSAFETY+ — Credits & Attribution

**Made in the USA** — built on older hardware, with care, and with **Grok Build (beta)** as a development contribution.

---

## Author

| | |
|---|---|
| **Creator / maintainer** | **medicinalsheep** |
| **Contact** | [jfkyt@icloud.com](mailto:jfkyt@icloud.com) |
| **Repository** | [github.com/medicinalsheep/IRLSAFETY](https://github.com/medicinalsheep/IRLSAFETY) |
| **Support development** | [GitHub Sponsors](https://github.com/sponsors/medicinalsheep?frequency=one-time&sponsor=medicinalsheep) — optional; IRLSAFETY+ stays free and 100% local |
| **License** | [MIT](LICENSE) |

---

## What this project is

**IRLSAFETY+** is a real-time privacy filter for OBS Studio. All live censorship runs **on your PC** — no cloud inference, no frame uploads.

The plugin watches your video sources, finds sensitive content (screen text, custom keywords, license plates, street signs, and pattern-matched numbers), and covers it before it reaches your stream or recording.

---

## Design (how it works)

```
Video source → IRLSAFETY+ filter
    ├─ Windows OCR (Screen Text, Custom PII, Sensitive Patterns)
    ├─ YOLOv8n ONNX detection (License Plates, Street Signs)
    ├─ Region tracker (overlays persist between scans; motion prediction)
    ├─ Censor compositor (solid box, blur, custom overlay image)
    ├─ Hybrid stream delay (baseline + auto buffer while protecting)
    └─ Secure mode (hold overlays; optional frame drop during motion)
```

**Privacy-first choices:**
- Inference is local (Windows.Media.Ocr + ONNX Runtime)
- Training is local (your disk, your labels, your GPU/CPU)
- Optional child OCR ONNX path for cross-platform builds later

**OBS integration:**
- Filter on each source (Display Capture, webcam, Media, etc.)
- Control dock for model reload, training hub, and setup walkthrough
- Bundled guides in `data/models/` (OCR, platforms, training, PII strategy)

---

## Training (v0.6.2 detection model)

The bundled **`irlsafety-detect.onnx`** model was trained in-house for US **license plates** and **street signs**:

| | |
|---|---|
| **Architecture** | YOLOv8n (Ultralytics) |
| **Classes** | `0 = license_plate`, `1 = street_sign` |
| **Data** | US bootstrap (LISA street signs + plate boxes) + local labeling |
| **Training** | 40 epochs on JWCOM-3 (CPU); checkpoints on JWCOM-4 RAM disk |
| **Export** | ONNX via Ultralytics export → `irlsafety-detect.onnx` (~12 MB) |
| **Peak metrics** | mAP50 ~0.988 (epoch 39); final mAP50 ~0.982 |

Re-train on your own GoPro/IRL frames with the Control dock or `scripts/train-model.ps1` — nothing leaves your machine.

---

## v0.7.0 (plugin — in progress)

| | |
|---|---|
| **Detection taxonomy** | 4 classes: `license_plate`, `street_sign`, `shipping_label`, `id_document` |
| **Angled cover** | Low-poly quad censor for OBB-trained models |
| **Settings** | Simplified Protection UI; overlap padding removed |
| **Training** | `train-model.ps1 -Device 0 -OBB`; RAM-disk / laptop guides |

Bundled ONNX may remain 2-class until the next local training run completes.

---

## Development contributions

| Contribution | Notes |
|--------------|-------|
| **medicinalsheep** | Architecture, pipeline, OBS plugin, training pipeline, US model |
| **Grok Build (beta)** | AI-assisted design, implementation, and iteration (v0.6–v0.7) |
| **OBS Plugin Template** | CMake/build scaffold ([obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)) |
| **Community references** | Patterns informed by [obs-detect](https://github.com/occ-ai/obs-detect) and [obs-ocr](https://github.com/occ-ai/obs-ocr) |

Third-party libraries and licenses: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

---

## Hardware note

This project was developed and trained on **older US hardware** — including CPU-only training runs where modern GPU stacks were unavailable — because privacy tooling should not require a datacenter to get started.

*Made in the USA with old hardware, love, and Grok Build beta.*