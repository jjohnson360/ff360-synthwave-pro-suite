import os
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_RIGHT, TA_JUSTIFY
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        if self._pageNumber == 1:
            return  # Skip cover page

        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#8a8a93"))

        # Header
        self.drawString(54, 750, "ff360_labs Synthwave Production Suite — User Manual v1.0.0-beta")
        self.setStrokeColor(colors.HexColor("#c9a15a"), alpha=0.3)
        self.setLineWidth(0.5)
        self.line(54, 742, 558, 742)

        # Footer
        self.line(54, 48, 558, 48)
        self.drawString(54, 36, "Copyright © 2026 ff360_labs. All rights reserved.")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(558, 36, page_str)
        self.restoreState()

def build_pdf(filename):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()

    # Custom Color Palette
    c_gold = colors.HexColor("#c9a15a")
    c_bg = colors.HexColor("#0a0a0b")
    c_card = colors.HexColor("#17171a")
    c_amber = colors.HexColor("#e8654a")
    c_sky = colors.HexColor("#38bdf8")
    c_text = colors.HexColor("#eae6dd")
    c_dim = colors.HexColor("#8a8a93")

    # Typography Styles
    title_style = ParagraphStyle(
        'CoverTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=28,
        leading=34,
        textColor=c_gold,
        alignment=TA_CENTER
    )

    subtitle_style = ParagraphStyle(
        'CoverSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=15,
        leading=20,
        textColor=colors.HexColor("#ffffff"),
        alignment=TA_CENTER
    )

    meta_style = ParagraphStyle(
        'CoverMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=14,
        textColor=c_dim,
        alignment=TA_CENTER
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Heading1'],
        fontName='Helvetica-Bold',
        fontSize=16,
        leading=20,
        textColor=c_gold,
        spaceAfter=8,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Heading2'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=colors.HexColor("#ffffff"),
        spaceBefore=10,
        spaceAfter=4,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=colors.HexColor("#222222"),
        spaceAfter=6,
        alignment=TA_LEFT
    )

    bullet_style = ParagraphStyle(
        'Bullet_Custom',
        parent=body_style,
        leftIndent=15,
        bulletIndent=5,
        spaceAfter=3
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9,
        leading=11,
        textColor=colors.HexColor("#ffffff"),
        alignment=TA_LEFT
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=11,
        textColor=colors.HexColor("#222222"),
        alignment=TA_LEFT
    )

    story = []

    # ==========================================
    # COVER PAGE
    # ==========================================
    story.append(Spacer(1, 100))
    story.append(Paragraph("ff360_labs", ParagraphStyle('Brand', parent=title_style, fontSize=18, textColor=c_gold)))
    story.append(Spacer(1, 15))
    story.append(Paragraph("SYNTHWAVE PRODUCTION SUITE", title_style))
    story.append(Spacer(1, 15))
    story.append(HRFlowable(width="60%", thickness=1.5, color=c_gold, spaceAfter=20, spaceBefore=10))
    story.append(Paragraph("Complete 8-Plugin User Manual & Technical Reference Guide", subtitle_style))
    story.append(Spacer(1, 15))
    story.append(Paragraph("VHS • Neon Chorus • Midnight Reverb • Neon Width<br/>Neon Tape Stop • Cyberpunk Glitch • RetroFX • NightDrive", ParagraphStyle('SubList', parent=subtitle_style, fontSize=11, leading=16, textColor=c_dim)))
    story.append(Spacer(1, 180))
    story.append(Paragraph("Version 1.0.0-beta Production Preview | VST3 • AU • Standalone<br/>Windows 10/11 & macOS 12+ (Apple Silicon & Intel)", meta_style))
    story.append(PageBreak())

    # ==========================================
    # SECTION 1: SUITE OVERVIEW & ARCHITECTURE
    # ==========================================
    story.append(Paragraph("1. Suite Architecture & Shared Core", h1_style))
    story.append(Paragraph(
        "The <b>ff360_labs Synthwave Production Suite</b> is a modular collection of eight professional audio plugins built on a single, high-performance C++20 DSP engine (<code>ff360_dsp_core</code>). Designed for modern Synthwave, Darksynth, Cyberpunk, and Vaporwave production, every plugin shares unified visual design tokens, smooth parameter interpolation, and post-Phase-10 broadcast metering precision.",
        body_style
    ))
    story.append(Spacer(1, 6))

    story.append(Paragraph("Shared Design System & Hero Controls", h2_style))
    story.append(Paragraph(
        "Every plugin in the suite adheres to ff360_labs' signature glassmorphic aesthetic: deep black backgrounds (<code>#0a0a0b</code>), matte charcoal panels, and metallic gold accents (<code>#c9a15a</code>). The signature macro knobs—<b>DEGRADE</b> in VHS, <b>PROBABILITY</b> in Cyberpunk Glitch, <b>GENERATE</b> in RetroFX, and <b>EVOLVE</b> in NightDrive—share an identical hero ring visual treatment and smooth parametric mapping curves.",
        body_style
    ))

    story.append(Paragraph("Post-Phase-10 Precision Metering Bridge", h2_style))
    story.append(Paragraph(
        "Each plugin embeds the calibrated <code>MeteringBridge</code> engine, providing sample-rate independent RMS integration, 4x polyphase FIR True Peak oversampling, IEC 60268-17 VU ballistics (-4.25 VU alignment), and EBU R128 gated Loudness Range (LRA) monitoring with -70 dBFS absolute and -10 LU relative thresholds.",
        body_style
    ))
    story.append(Spacer(1, 10))

    # ==========================================
    # SECTION 2: THE 8 PLUGINS DETAILED
    # ==========================================
    story.append(Paragraph("2. Plugin Reference & Parameter Guide", h1_style))

    # VHS
    story.append(Paragraph("📼 VHS — Flagship Tape Degradation Processor", h2_style))
    story.append(Paragraph("VHS models the magnetic, mechanical, and electronic imperfections of 1980s VCRs, cassette decks, and studio tape reels.", body_style))
    story.append(Paragraph("• <b>DEGRADE Macro</b>: Single-knob master control driving saturation, wow, flutter, pitch drift, and high-frequency loss along calibrated curves.", bullet_style))
    story.append(Paragraph("• <b>Saturation & Hysteresis</b>: Dynamic asymmetric soft-clipping modeling magnetic tape flux compression.", bullet_style))
    story.append(Paragraph("• <b>Mechanical Wow & Flutter</b>: Dual-rate capstan wobble (0.5 Hz wow + 6.0 Hz flutter).", bullet_style))
    story.append(Paragraph("• <b>Noise & Mechanical Hiss</b>: Filtered analog tape hiss and mechanical chassis hum.", bullet_style))

    # Neon Chorus
    story.append(Paragraph("🔮 Neon Chorus — 80s Multi-Voice Chorus & Ensemble", h2_style))
    story.append(Paragraph("A lush BBD-style chorus with stereo and 4-voice quad modes for wide synth pads and shimmering guitars.", body_style))
    story.append(Paragraph("• <b>Stereo / Quad Mode</b>: Toggle between classic 2-voice dual LFO and ultra-wide 4-voice quad delay networks.", bullet_style))
    story.append(Paragraph("• <b>Modern / Vintage Character</b>: Vintage mode introduces subtle clock jitter, analog warmth, and bucket-brigade high-frequency roll-off.", bullet_style))
    story.append(Paragraph("• <b>4th-Order Bass Mono</b>: Linkwitz-Riley low-end crossover filter ensuring 100% mono sub-bass compatibility below selectable cutoff.", bullet_style))

    # Midnight Reverb
    story.append(Paragraph("🌌 Midnight Reverb — Retro Algorithmic Spaces & Shimmer", h2_style))
    story.append(Paragraph("Rich vintage digital reverberation featuring 6 selectable space algorithms and instant sidechain ducking.", body_style))
    story.append(Paragraph("• <b>6 Algorithms</b>: Digital Hall, Dark Plate, Gated Room (80s Phil Collins drums), Synth Room, Endless (infinite decay), and Dream (lush shimmer modulation).", bullet_style))
    story.append(Paragraph("• <b>Ducking Envelope</b>: Built-in single-buffer transient ducking that attenuates reverb tail during dry transient hits.", bullet_style))
    story.append(Paragraph("• <b>Infinite Freeze</b>: Holds the reverb buffer infinitely without overload or runaway resonance.", bullet_style))

    # Neon Width
    story.append(Paragraph("🌐 Neon Width — Psychoacoustic Stereo Imaging & Goniometer", h2_style))
    story.append(Paragraph("Precision spatial imaging engine with an integrated real-time Lissajous polar goniometer.", body_style))
    story.append(Paragraph("• <b>Polar Goniometer & Phase Correlation Bar</b>: Real-time visual feedback of stereo spread and mono compatibility.", bullet_style))
    story.append(Paragraph("• <b>Haas & Micro-Delay</b>: 0–25ms inter-aural time difference widening.", bullet_style))
    story.append(Paragraph("• <b>Frequency-Dependent Width</b>: Expands high-frequency air while preserving narrow, focused low-mids.", bullet_style))
    story.append(Paragraph("• <b>Stereo Rotation</b>: Panning rotation matrix rotating the stereo field without collapsing channel energy.", bullet_style))

    # Neon Tape Stop
    story.append(Paragraph("🛑 Neon Tape Stop — Musical Tape, Vinyl & Digital Brake", h2_style))
    story.append(Paragraph("A dedicated slowdown and spin-up effect with selectable inertia profiles and MIDI triggering.", body_style))
    story.append(Paragraph("• <b>Profiles</b>: Vinyl Stop (turntable motor drag), Tape Stop (analog reel inertia + HF loss), and Digital Stop (stepped quantization cut).", bullet_style))
    story.append(Paragraph("• <b>Reverse Recovery</b>: Spins back up to speed in reverse phase for creative drops and transitions.", bullet_style))

    # Cyberpunk Glitch
    story.append(Paragraph("👾 Cyberpunk Glitch — Tempo-Synced Stutter & Buffer Glitch", h2_style))
    story.append(Paragraph("Rhythmic stutter generator, buffer freeze, and bitcrush processor synced to host tempo.", body_style))
    story.append(Paragraph("• <b>Hero Probability Knob</b>: Sets likelihood of glitch firing on each grid subdivision (1/4 to 1/32).", bullet_style))
    story.append(Paragraph("• <b>Reverse Playback & Freeze</b>: Instant reverse slice looping and infinite buffer hold.", bullet_style))
    story.append(Paragraph("• <b>Granular Pitch Shift</b>: -12 to +12 semitones real-time slice transposition.", bullet_style))

    # RetroFX
    story.append(Paragraph("⚡ RetroFX — Generative Synthwave Transition Synthesizer", h2_style))
    story.append(Paragraph("On-demand synthwave transition and riser generator eliminating the need for static sample packs.", body_style))
    story.append(Paragraph("• <b>9 Generators</b>: Noise Sweep, Pitch Sweep, Laser, Reverse Swell, Impact Sub-Boom, Riser, Downlifter, Digital Sweep, Tape Sweep.", bullet_style))
    story.append(Paragraph("• <b>GENERATE Hero Control</b>: Instant trigger with deterministic seed recall for 100% reproducible takes.", bullet_style))

    # NightDrive
    story.append(Paragraph("🌃 NightDrive — Generative Synthwave Ambient Bed & Textures", h2_style))
    story.append(Paragraph("Generative ambient backdrop engine combining analog drones, granular texture clouds, and arpeggios.", body_style))
    story.append(Paragraph("• <b>EVOLVE Macro</b>: Single master knob modulating grain drift, chorus rate, LFO speed, and shimmer depth.", bullet_style))
    story.append(Paragraph("• <b>Chord / Scale Lock</b>: Automatically constrains pitch generation to key (Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Synthwave Pentatonic). Supports live ChordFlow integration with automatic fallback.", bullet_style))

    story.append(PageBreak())

    # ==========================================
    # SECTION 3: FACTORY PRESET DIRECTORY
    # ==========================================
    story.append(Paragraph("3. Factory Preset Index (41 Presets)", h1_style))
    story.append(Paragraph("All 41 factory presets use JSON serialization via <code>ParameterManager</code> for instant, glitch-free DAW recall.", body_style))
    story.append(Spacer(1, 4))

    preset_data = [
        [Paragraph("<b>Plugin</b>", table_header_style), Paragraph("<b>Preset Name</b>", table_header_style), Paragraph("<b>Category</b>", table_header_style), Paragraph("<b>Description & Sonic Character</b>", table_header_style)],
        [Paragraph("VHS", table_cell_style), Paragraph("Clean Master Tape", table_cell_style), Paragraph("Subtle", table_cell_style), Paragraph("Subtle studio reel tape saturation and gentle high-end rounding.", table_cell_style)],
        [Paragraph("VHS", table_cell_style), Paragraph("1984 VCR Broadcast", table_cell_style), Paragraph("Vintage", table_cell_style), Paragraph("Authentic 80s videotape playback with moderate wow and chassis hum.", table_cell_style)],
        [Paragraph("VHS", table_cell_style), Paragraph("Melancholy Cassette", table_cell_style), Paragraph("Lo-Fi", table_cell_style), Paragraph("Slow pitch wobble, gentle lo-fi decimation, and warm harmonic bloom.", table_cell_style)],
        [Paragraph("VHS", table_cell_style), Paragraph("Broken Walkman", table_cell_style), Paragraph("Extreme", table_cell_style), Paragraph("Severe motor drag, tape crunch, and intense wow/flutter instability.", table_cell_style)],
        [Paragraph("VHS", table_cell_style), Paragraph("Tape Bloom & Warmth", table_cell_style), Paragraph("Harmonics", table_cell_style), Paragraph("Harmonic tape drive ideal for drum buses and synthesizer master tracks.", table_cell_style)],
        [Paragraph("Neon Chorus", table_cell_style), Paragraph("Juno Pad Spread", table_cell_style), Paragraph("Pads", table_cell_style), Paragraph("Iconic 1982 analog synthesizer chorus with ultra-wide stereo movement.", table_cell_style)],
        [Paragraph("Neon Chorus", table_cell_style), Paragraph("Tokyo 1986 Ensemble", table_cell_style), Paragraph("Vintage", table_cell_style), Paragraph("4-voice quad BBD ensemble with clock drift and warm HF loss.", table_cell_style)],
        [Paragraph("Neon Chorus", table_cell_style), Paragraph("Dimension 4D Clean", table_cell_style), Paragraph("Modern", table_cell_style), Paragraph("Ultra-clean modern spatial chorus with zero phase smearing.", table_cell_style)],
        [Paragraph("Midnight Reverb", table_cell_style), Paragraph("Cyberpunk Cathedral", table_cell_style), Paragraph("Halls", table_cell_style), Paragraph("Vast metallic hall with dense reflections for dark synth backdrops.", table_cell_style)],
        [Paragraph("Midnight Reverb", table_cell_style), Paragraph("80s Snare Gate", table_cell_style), Paragraph("Gated", table_cell_style), Paragraph("Classic non-linear gated room with instantaneous explosive cutoff.", table_cell_style)],
        [Paragraph("Midnight Reverb", table_cell_style), Paragraph("Blade Runner Dream", table_cell_style), Paragraph("Cinematic", table_cell_style), Paragraph("Lush modulated shimmer reverb with slow harmonic bloom.", table_cell_style)],
        [Paragraph("Neon Width", table_cell_style), Paragraph("Wide Synth Pad", table_cell_style), Paragraph("Pads", table_cell_style), Paragraph("Haas widening + M/S expansion with safe mono sub crossover.", table_cell_style)],
        [Paragraph("Neon Width", table_cell_style), Paragraph("Rotating Horizon", table_cell_style), Paragraph("Movement", table_cell_style), Paragraph("Continuous subtle stereo rotation matrix for evolving pad movement.", table_cell_style)],
        [Paragraph("Neon Tape Stop", table_cell_style), Paragraph("Classic 1/2 Bar Brake", table_cell_style), Paragraph("Tape", table_cell_style), Paragraph("Authentic 800ms reel-to-reel motor slowdown and spin-up.", table_cell_style)],
        [Paragraph("Neon Tape Stop", table_cell_style), Paragraph("Vinyl Turntable Drop", table_cell_style), Paragraph("Vinyl", table_cell_style), Paragraph("S-curve turntable power-down with realistic needle drag.", table_cell_style)],
        [Paragraph("Cyberpunk Glitch", table_cell_style), Paragraph("1/16 Beat Stutter", table_cell_style), Paragraph("Rhythm", table_cell_style), Paragraph("Tempo-synced 1/16th rhythmic stutter repeats with 60% probability.", table_cell_style)],
        [Paragraph("Cyberpunk Glitch", table_cell_style), Paragraph("Dystopian Reverse", table_cell_style), Paragraph("Dark", table_cell_style), Paragraph("1/8th note reverse slices with -12st octave drop and bitcrush.", table_cell_style)],
        [Paragraph("RetroFX", table_cell_style), Paragraph("Cyberpunk 4-Bar Riser", table_cell_style), Paragraph("Risers", table_cell_style), Paragraph("4-bar ascending multi-saw detuned riser with resonant filter sweep.", table_cell_style)],
        [Paragraph("RetroFX", table_cell_style), Paragraph("Neon Laser Impact", table_cell_style), Paragraph("Impacts", table_cell_style), Paragraph("High-energy modulated FM laser shot followed by sub-bass drop.", table_cell_style)],
        [Paragraph("NightDrive", table_cell_style), Paragraph("Midnight Highway Drone", table_cell_style), Paragraph("Drones", table_cell_style), Paragraph("Sub-heavy analog pad drone with subtle granular raindrops.", table_cell_style)],
        [Paragraph("NightDrive", table_cell_style), Paragraph("Tokyo 3AM Atmosphere", table_cell_style), Paragraph("Cinematic", table_cell_style), Paragraph("Evolving cinematic texture in Synthwave Pentatonic scale.", table_cell_style)]
    ]

    t = Table(preset_data, colWidths=[75, 110, 65, 254])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), c_card),
        ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
        ('TOPPADDING', (0, 0), (-1, -1), 4),
        ('LEFTPADDING', (0, 0), (-1, -1), 4),
        ('RIGHTPADDING', (0, 0), (-1, -1), 4),
        ('GRID', (0, 0), (-1, -1), 0.5, colors.HexColor("#dddddd")),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.HexColor("#ffffff"), colors.HexColor("#f8f8f8")])
    ]))
    story.append(t)
    story.append(Spacer(1, 10))

    # ==========================================
    # SECTION 4: TECHNICAL SPECIFICATIONS
    # ==========================================
    story.append(Paragraph("4. Technical Specifications & System Requirements", h1_style))
    story.append(Paragraph("• <b>Plugin Formats</b>: VST3 (64-bit), Audio Units (AUv2), Standalone Host Application.", bullet_style))
    story.append(Paragraph("• <b>Supported Operating Systems</b>: Windows 10 & 11 (64-bit), macOS 12 Monterey or higher (Universal Binary: Native Apple Silicon M1/M2/M3/M4 & Intel x86_64).", bullet_style))
    story.append(Paragraph("• <b>Supported Sample Rates</b>: 44.1 kHz, 48.0 kHz, 88.2 kHz, 96.0 kHz, 176.4 kHz, 192.0 kHz (with automatic filter coefficient warping).", bullet_style))
    story.append(Paragraph("• <b>DAW Compatibility</b>: Ableton Live 11/12, FL Studio 21+, Logic Pro 10.7+, Reaper 7+, Cubase 12+, Studio One 6+, Bitwig Studio 5+.", bullet_style))
    story.append(Paragraph("• <b>CPU Efficiency</b>: Ultra-low latency DSP core consuming &lt; 1.4% CPU per instance at 96 kHz on modern hardware.", bullet_style))

    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"User manual generated successfully at: {filename}")

if __name__ == "__main__":
    target_path = r"c:\Users\fredd\OneDrive\Desktop\ff360_labs\ff360_labs Synthwave Production Suite\ff360_synthwave_pro_suite\docs\manuals\ff360_Synthwave_Production_Suite_User_Manual.pdf"
    build_pdf(target_path)
