# Input Streamliner - Product Brief

## Executive Summary

Input Streamliner is an Unreal Engine 5 editor plugin that dramatically simplifies multiplatform input configuration. By combining an intuitive interface with AI-powered natural language understanding, developers can generate complete input systems—including Enhanced Input assets, platform-specific bindings, mobile touch controls, and runtime rebinding UI—in minutes rather than hours.

---

## Problem Statement

### The Pain

Setting up input in Unreal Engine 5 is tedious, error-prone, and repetitive:

1. **Verbose Setup Ceremony**: Creating a functional input system requires manually creating Input Actions, Input Mapping Contexts, configuring triggers and modifiers, and wiring bindings in Blueprint or C++. A simple "WASD + Jump" setup touches 5+ assets and dozens of configuration fields.

2. **Multiplatform Complexity**: Each platform (PC keyboard/mouse, gamepad, iOS, Android) needs its own Input Mapping Context with appropriate bindings. Developers often ship with incomplete platform support or inconsistent controls.

3. **Mobile is Particularly Painful**: Touch controls require creating UMG widgets for virtual joysticks, buttons, and gesture detection—completely separate from the Enhanced Input workflow. Most developers defer mobile input until late in development, leading to rushed implementations.

4. **Rebinding is an Afterthought**: Runtime key rebinding is expected by players but requires significant custom UI and persistence work. Many shipped games have poor or missing rebind functionality.

5. **No Single Source of Truth**: Input configuration is scattered across multiple asset types with no unified view of "what inputs does my game support?"

### User Impact

- **Indie developers** spend disproportionate time on input boilerplate instead of gameplay
- **Teams** have inconsistent input implementations across platforms
- **Players** receive subpar experiences (missing rebinding, poor mobile controls)
- **Prototyping** is slowed because basic input setup takes too long

### Evidence

- Frequent forum/Discord questions about Enhanced Input setup complexity
- Marketplace assets for "input managers" and "mobile controls" indicate unmet needs
- Common complaint: "I just want WASD movement, why is this so many steps?"

---

## Solution

### Core Value Proposition

**Define your inputs once in plain English. Generate everything else.**

Input Wizard provides:

1. **Natural Language Input Definition**: Describe your input needs conversationally ("third-person action game with dodge roll, lock-on, and weapon wheel") and receive a complete, correctly-configured input system.

2. **Unified Input Dashboard**: Single interface showing all actions, their bindings across platforms, and generation status.

3. **One-Click Multiplatform**: Select target platforms; the tool generates appropriate Input Mapping Contexts with sensible defaults for each.

4. **Mobile-First Touch Controls**: Automatically generates UMG widgets for virtual joysticks, buttons, D-pads, radial menus, and gesture handlers—properly integrated with Enhanced Input.

5. **Rebinding UI Generation**: Produces a complete, functional settings menu for runtime input rebinding with conflict detection, reset-to-defaults, and save/load.

6. **C++ or Blueprint Output**: Developer chooses their preferred binding approach; the tool generates the appropriate code.

---

## Target Users

### Primary: Solo/Small Team Developers

- Building multiplatform games
- Limited time for boilerplate
- May lack deep Enhanced Input expertise
- Value speed and correctness

### Secondary: Mid-Size Studios

- Need consistent input implementation across team members
- Want to enforce input standards
- Prototyping multiple projects simultaneously

### Tertiary: Technical Designers

- Rapid prototyping
- Experimenting with control schemes
- Less comfortable with C++

---

## User Stories

1. **As a developer**, I want to describe my game's inputs in natural language so that I can get a working input system without memorizing Enhanced Input's configuration options.

2. **As a developer**, I want to select my target platforms and have appropriate bindings generated so that I don't have to research platform-specific conventions.

3. **As a developer**, I want touch controls generated automatically so that mobile support doesn't require separate widget development.

4. **As a developer**, I want a rebinding UI generated so that players can customize controls without me building settings menus from scratch.

5. **As a developer**, I want to preview my input configuration before generating so that I can catch mistakes early.

6. **As a developer**, I want to choose between C++ and Blueprint output so that the generated code fits my project's architecture.

---

## Feature Scope

### Phase 1: Core Input Streamliner (MVP)

| Feature | Description | Priority |
|---------|-------------|----------|
| Action Definition UI | Form-based input action creation | P0 |
| Dynamic Add/Remove | Add, remove, update actions at any time | P0 |
| Natural Language Parsing | LLM converts descriptions to structured actions | P0 |
| Input Action Generation | Creates UInputAction assets | P0 |
| Mapping Context Generation | Creates UInputMappingContext per platform | P0 |
| Platform Presets | PC, Mac, Gamepad, iOS, Android defaults | P0 |
| Undo/Redo Support | Full editor undo/redo for all operations | P0 |
| Genre Templates | FPS, Third-Person, Platformer, Top-Down presets | P1 |
| Preview Panel | Shows what will be generated before commit | P1 |
| C++ Binding Generation | Generates input binding code | P1 |
| Blueprint Binding Generation | Generates BP nodes/graphs | P1 |
| Asset Synchronization | Keep generated assets in sync with config | P1 |

### Phase 2: Mobile Touch Controls

| Feature | Description | Priority |
|---------|-------------|----------|
| Virtual Joystick Widget | Fixed and floating variants | P0 |
| Virtual Button Widget | Configurable touch buttons | P0 |
| D-Pad Widget | 4/8 directional discrete input | P1 |
| Radial Menu Widget | Weapon wheel / ability selector | P1 |
| Gesture Handlers | Tap, swipe, pinch, rotate detection | P1 |
| Gyroscope Integration | Motion aiming with calibration | P2 |
| Touch Region Mapping | Invisible zones for screen areas | P1 |
| Layout Editor | Drag-to-reposition touch controls | P2 |
| Safe Area Handling | Notch/home indicator avoidance | P0 |

### Phase 3: Rebinding System

| Feature | Description | Priority |
|---------|-------------|----------|
| Rebinding UI Widget | Generated settings menu | P0 |
| Key Capture System | "Press any key" listener | P0 |
| Conflict Detection | Warns on duplicate bindings | P0 |
| Reset to Defaults | Per-action and global reset | P1 |
| Save/Load Bindings | Persistence to local storage | P0 |
| Sensitivity Sliders | Mouse, stick, gyro sensitivity | P1 |
| Gyro Calibration UI | Hold-to-calibrate flow | P2 |
| Touch Layout Customization | Player-adjustable control positions | P2 |

### Phase 4: Advanced Features (Future)

| Feature | Description | Priority |
|---------|-------------|----------|
| Input Visualization | Debug overlay showing active inputs | P2 |
| Analytics Hooks | Track which inputs players use | P3 |
| Accessibility Presets | One-handed, reduced motion modes | P2 |
| Import/Export | Share input configs between projects | P3 |

---

## Success Metrics

### Adoption

| Metric | Target | Measurement |
|--------|--------|-------------|
| Plugin downloads (if released) | 1,000+ first month | Marketplace/GitHub |
| Demo video views | 10,000+ | YouTube/social |
| Community feedback | Net positive sentiment | Forums/Discord |

### Efficiency

| Metric | Target | Measurement |
|--------|--------|-------------|
| Time to basic input system | < 2 minutes | User testing |
| Time to multiplatform setup | < 5 minutes | User testing |
| Time to mobile controls | < 10 minutes | User testing |
| Reduction vs manual setup | 80%+ time saved | Comparative testing |

### Quality

| Metric | Target | Measurement |
|--------|--------|-------------|
| Generated code correctness | 100% compiles | Automated testing |
| Platform parity | All platforms functional | Manual QA |
| Rebinding reliability | Zero data loss | Automated testing |

---

## Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| LLM misinterprets input descriptions | Medium | Medium | Few-shot prompting, structured output validation, manual override always available |
| Generated code doesn't match project conventions | Medium | Low | Code style options, template customization |
| Mobile touch controls conflict with game UI | Medium | Medium | Z-order configuration, integration guidelines |
| Enhanced Input API changes in future UE versions | Low | High | Abstract generation layer, version-specific templates |
| Local LLM performance varies by hardware | Medium | Low | Document requirements, offer cloud fallback option |

---

## Technical Constraints

- **Unreal Engine 5.3+** required (Enhanced Input maturity)
- **Local LLM** requires 8GB+ VRAM for optimal performance (Nemotron 8B)
- **Editor-only** tool; generated assets work at runtime
- **Windows/Mac** editor support initially

---

## Competitive Landscape

| Solution | Strengths | Weaknesses |
|----------|-----------|------------|
| Manual Enhanced Input | Full control | Tedious, error-prone |
| Marketplace input managers | Working code | Not AI-powered, limited customization |
| Common UI plugin | Official, robust | Doesn't generate, still requires setup |
| **Input Streamliner** | AI-powered, generates everything, multiplatform | New, unproven |

---

## Roadmap

```
Q1 2025: Phase 1 - Core Wizard
├── Week 1-2: Plugin skeleton, basic UI
├── Week 3-4: Input Action/Mapping Context generation
├── Week 5-6: LLM integration, natural language parsing
└── Week 7-8: Platform presets, genre templates

Q2 2025: Phase 2 - Mobile
├── Week 1-3: Touch control widgets
├── Week 4-5: Gesture system
└── Week 6-8: Gyro, layout editor

Q3 2025: Phase 3 - Rebinding
├── Week 1-3: Rebinding UI generation
├── Week 4-5: Persistence, conflict handling
└── Week 6-8: Polish, accessibility

Q4 2025: Phase 4 - Advanced
├── Analytics, visualization
└── Import/export, community features
```

---

## Open Questions

1. Should the tool support legacy Input System alongside Enhanced Input?
2. What's the right default behavior when LLM parsing fails—error or best-guess?
3. Should generated widgets use CommonUI for platform-adaptive styling?
4. Is there demand for console-specific support (PlayStation, Xbox, Switch)?

---

## Appendix: User Research Signals

### Forum/Community Themes

- "Enhanced Input is powerful but the setup is overwhelming"
- "I wish there was a visual tool for mapping inputs"
- "Mobile controls are always an afterthought"
- "Every project I rebuild the same input boilerplate"

### Marketplace Signals

- Multiple "Easy Input" plugins exist, indicating demand
- Mobile control packs sell consistently
- Rebinding system assets have positive reviews

---

*Document Version: 1.0*
*Last Updated: January 2025*
*Author: [Your Name]*
