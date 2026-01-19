# Input Streamliner

An Unreal Engine 5.7 editor plugin that dramatically simplifies multiplatform input configuration using AI-powered natural language understanding.

## Overview

Input Streamliner allows developers to generate complete input systems—including Enhanced Input Actions, Input Mapping Contexts, platform-specific bindings, and mobile touch controls—in minutes rather than hours.

### Why Input Streamliner?

**Input Streamliner is the only tool of its kind.** While there are several AI-powered plugins for Unreal Engine (like TotalAI, ClaudeAI Plugin, and UnrealGenAISupport), they focus on general Blueprint generation, C++ assistance, or scene creation. None offer a turnkey solution for input system generation.

Other approaches require you to either:
- Manually create dozens of Input Actions and Mapping Contexts by hand
- Write custom prompts for general-purpose AI tools and hope the output is correct
- Follow lengthy tutorials to set up Enhanced Input from scratch

Input Streamliner is purpose-built for one thing: **describe your controls in plain English, get a complete, correctly-configured input system.**

### Key Features

- **Natural Language Input Definition** - Describe your input needs conversationally and receive a complete, correctly-configured input system
- **Multiplatform Support** - PC (Keyboard/Mouse), Gamepad, Mac, iOS, Android
- **Mobile Touch Controls** - Virtual joysticks, buttons, swipe zones, gesture detection
- **Advanced Input Settings** - Auto-configured DeadZones, triggers (Hold, Tap, DoubleTap, Released), and axis modifiers
- **Model Selection** - Choose from any locally installed Ollama model via dropdown
- **Asset Management** - Generate and delete input assets with one click
- **Runtime Rebinding** - Built-in player key rebinding system with persistence

## Requirements

- Unreal Engine 5.7+
- [Ollama](https://ollama.ai/) running locally (for AI features)
- A compatible LLM model (recommended: `llama3`, `mistral`, or `codellama`)

## Installation

1. Clone or download this repository
2. Copy the `Plugins/InputStreamliner` folder to your project's `Plugins` directory
3. Rebuild your project or launch the editor (it will prompt to compile)

## Quick Start

### 1. Install and Start Ollama

```bash
# Install Ollama from https://ollama.ai/
# Then pull a model:
ollama pull llama3

# Start Ollama (runs on localhost:11434 by default)
ollama serve
```

### 2. Open the Input Streamliner Widget

1. In Unreal Editor, go to **Content Browser**
2. Navigate to `Plugins/InputStreamliner Content/`
3. Right-click on `EUW_StreamlineInput` (or your Editor Utility Widget)
4. Select **Run Editor Utility Widget**

### 3. Configure and Generate

1. **Test Connection** - Click "Test LLM" to verify Ollama is running
2. **Select Model** - Use the dropdown to choose your preferred Ollama model
3. **Set Prefix** - Enter your project prefix (e.g., "MyGame")
4. **Describe Inputs** - Enter a natural language description of your input needs
5. **Parse** - Click "Parse" to let the AI generate input definitions
6. **Generate** - Click "Generate" to create the actual UE5 assets

### Example Prompts

**Basic FPS Controls:**
```
I need WASD movement, mouse look, spacebar to jump, left shift to sprint,
left mouse to shoot, right mouse to aim, R to reload, and E to interact.
```

**Mobile Third-Person Game:**
```
Mobile third-person game controls:
- Left virtual joystick for character movement (Axis2D)
- Right swipe area for camera look (Axis2D)
- Tap right side for attack (Bool)
- Double tap to dodge (Bool)
- Hold for block (Bool)
- Swipe up to jump (Bool)
```

**Racing Game:**
```
Racing game with triggers for throttle and brake (Axis1D),
left stick for steering, A button for nitro boost,
B button for handbrake, and Y to look behind.
```

## Generated Assets

The plugin generates:

| Asset Type | Location | Naming Convention |
|------------|----------|-------------------|
| Input Actions | `/Game/Input/Actions/` | `IA_ActionName` |
| Mapping Contexts | `/Game/Input/Contexts/` | `IMC_PC_Keyboard`, `IMC_PC_Gamepad`, `IMC_iOS`, etc. |

## Features in Detail

### Automatic Modifiers

- **DeadZone** - Automatically added to gamepad sticks (0.2 radial) and triggers (0.1 axial)
- **Negate** - Applied for negative axis directions (A/D, S keys)
- **SwizzleAxis** - Applied for Y-axis keyboard inputs (W/S keys)

### Trigger Types

| Trigger | Description |
|---------|-------------|
| Pressed | Fires once when pressed (default) |
| Released | Fires when released |
| Hold | Fires after holding for 0.2s |
| Tap | Fires on quick tap (<0.2s) |
| DoubleTap | Fires on double tap |

### Platform Contexts

Separate Input Mapping Contexts are generated for each platform:
- `IMC_PC_Keyboard` - Keyboard and mouse bindings
- `IMC_PC_Gamepad` - Xbox/PlayStation controller bindings
- `IMC_iOS` - iOS touch control references
- `IMC_Android` - Android touch control references
- `IMC_Mac` - Mac keyboard/mouse bindings

### Runtime Key Rebinding

The plugin includes a complete runtime rebinding system (`UInputRebindingManager`) that allows players to customize their controls:

- **Input Capture** - Captures keyboard, gamepad, mouse buttons, and scroll wheel
- **Conflict Detection** - Warns when a key is already bound to another action
- **JSON Persistence** - Saves custom bindings to `Saved/InputStreamliner/Bindings.json`
- **Sensitivity Settings** - Mouse, gamepad, and gyroscope sensitivity with invert Y option
- **Blueprint Exposed** - All functions callable from Blueprints for easy UI integration

```cpp
// Example: Start rebinding an action
UInputRebindingManager* Manager = GetGameInstance()->GetSubsystem<UInputRebindingManager>();
Manager->StartRebinding(MyInputAction);

// Listen for completion
Manager->OnRebindComplete.AddDynamic(this, &UMyWidget::OnRebindFinished);
```

## Configuration

Default LLM settings (can be modified in `InputStreamlinerConfiguration`):
- **Endpoint**: `http://localhost`
- **Port**: `11434`
- **Model**: `llama3`

## Troubleshooting

### "Parse Failed: Failed to parse JSON"
- The LLM may have returned malformed JSON. Try a different model or simplify your prompt.
- Check Output Log (filter by `LogInputStreamliner`) to see the raw LLM response.

### "Connection failed"
- Ensure Ollama is running (`ollama serve`)
- Check that port 11434 is not blocked
- Verify the model is downloaded (`ollama list`)

### Model dropdown is empty
- Ollama may not be running or accessible
- Check the Output Log for connection errors

## Project Structure

```
Plugins/InputStreamliner/
├── Source/
│   ├── InputStreamliner/           # Editor module
│   │   ├── Public/
│   │   │   ├── InputStreamlinerWidget.h
│   │   │   ├── LLMIntentParser.h
│   │   │   ├── InputAssetGenerator.h
│   │   │   └── ...
│   │   └── Private/
│   │       └── ...
│   └── InputStreamlinerRuntime/    # Runtime module
│       ├── Public/
│       │   └── InputRebindingManager.h
│       └── Private/
│           └── InputRebindingManager.cpp
└── Content/
    └── EUW_StreamlineInput.uasset
```

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions welcome! Please open an issue or submit a pull request.
