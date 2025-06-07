# LED Strip JSON API - Complete Documentation

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [JSON Structure](#json-structure)
4. [Command Reference](#command-reference)
5. [Color Specifications](#color-specifications)
6. [Effect Types](#effect-types)
7. [Transition Types](#transition-types)
8. [Control Flow](#control-flow)
9. [Examples](#examples)
10. [Error Handling](#error-handling)
11. [Best Practices](#best-practices)
12. [Troubleshooting](#troubleshooting)

---

## Overview

The LED Strip JSON API provides a comprehensive interface for controlling WS2812B/NeoPixel LED strips on ESP32 microcontrollers. The API supports real-time color control, smooth transitions, visual effects, gradients, and complex sequencing with looping capabilities.

### Key Features
- **Individual Pixel Control**: Set colors for specific LEDs
- **Fill Operations**: Set all LEDs to the same color
- **Gradient Generation**: Create smooth color transitions across the strip
- **Visual Effects**: Built-in animations like rainbow, breathing, fire, etc.
- **Smooth Transitions**: Animated changes between states
- **Command Sequencing**: Chain multiple commands with `then` arrays
- **Looping**: Repeat command sequences indefinitely
- **Memory Management**: Safe allocation and cleanup of command sequences

---

## Core Concepts

### Command Processing
Commands are processed sequentially within a single JSON object. When multiple commands are present, they execute in this order:
1. `gradient` commands
2. `effect` commands
3. `fill` commands
4. `pixels` commands
5. `animation` commands

### Transition System
The API supports smooth transitions between states using various easing functions. Transitions can be applied to:
- Fill operations
- Gradient changes
- Effect modifications
- Individual effect parameters

### Memory Management
The system uses dynamic memory allocation for storing command sequences and implements safety measures:
- Maximum nesting depth of 10 levels
- Memory allocation checks with fallback handling
- Automatic cleanup of completed sequences
- Size limits on JSON structures to prevent memory exhaustion

---

## JSON Structure

### Basic Command Structure
```json
{
  "command_type": {
    "parameter": "value"
  },
  "then": [
    {"next_command": "..."},
    {"another_command": "..."}
  ],
  "loop": true
}
```

### Root Level Properties
- **Commands**: `fill`, `gradient`, `effect`, `pixels`, `animation`
- **Control Flow**: `then`, `loop`
- **Timing**: Various `transitionDuration` and timing parameters

---

## Command Reference

## Fill Command

Controls the overall color of the LED strip.

### Syntax
```json
{
  "fill": {
    "color": <color_specification>,
    "transitionDuration": <milliseconds>,
    "transitionType": "<transition_name>"
  }
}
```

### Parameters
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `color` | Color Object/String | Yes | Target color for all LEDs |
| `transitionDuration` | Number | No | Transition time in milliseconds |
| `transitionType` | String | No | Easing function (default: "ease_in_out") |

### Examples
```json
// Instant red fill
{
  "fill": {
    "color": "red"
  }
}

// Smooth transition to blue over 2 seconds
{
  "fill": {
    "color": "#0000FF",
    "transitionDuration": 2000,
    "transitionType": "ease_out"
  }
}

// RGB color with alpha
{
  "fill": {
    "color": {
      "r": 255,
      "g": 128,
      "b": 0,
      "a": 200
    }
  }
}
```

---

## Gradient Command

Creates color gradients across the LED strip.

### Syntax
```json
{
  "gradient": {
    "start": <color>,
    "end": <color>,
    "stops": [<gradient_stops>],
    "reverse": <boolean>,
    "smooth": <boolean>,
    "duration": <milliseconds>,
    "easing": "<transition_name>",
    "enabled": <boolean>,
    "clear": <boolean>
  }
}
```

### Parameters
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | Color | Conditional | Start color for two-color gradient |
| `end` | Color | Conditional | End color for two-color gradient |
| `stops` | Array | Conditional | Multi-stop gradient definition |
| `reverse` | Boolean | No | Reverse gradient direction |
| `smooth` | Boolean | No | Enable smooth transition |
| `duration` | Number | No | Transition duration (when smooth=true) |
| `easing` | String | No | Transition easing function |
| `enabled` | Boolean | No | Enable/disable gradient overlay |
| `clear` | Boolean | No | Clear current gradient |

### Gradient Stop Object
```json
{
  "color": <color_specification>,
  "position": <float_0_to_1>
}
```

### Examples
```json
// Simple two-color gradient
{
  "gradient": {
    "start": "red",
    "end": "blue"
  }
}

// Multi-stop gradient with smooth transition
{
  "gradient": {
    "stops": [
      {"color": "red", "position": 0.0},
      {"color": "yellow", "position": 0.5},
      {"color": "blue", "position": 1.0}
    ],
    "smooth": true,
    "duration": 3000,
    "easing": "ease_in_out"
  }
}

// Reversed gradient
{
  "gradient": {
    "start": "purple",
    "end": "cyan",
    "reverse": true
  }
}

// Clear gradient
{
  "gradient": {
    "clear": true
  }
}
```

---

## Effect Command

Applies animated visual effects to the LED strip.

### Syntax
```json
{
  "effect": {
    "type": "<effect_name>",
    "speed": <float>,
    "intensity": <float>,
    "colors": [<color_array>],
    "transitionDuration": <milliseconds>,
    "transitionType": "<transition_name>"
  }
}
```

### Parameters
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `type` | String | Yes | Effect type name |
| `speed` | Float | No | Effect speed (0.1 - 10.0) |
| `intensity` | Float | No | Effect intensity (0.0 - 2.0) |
| `colors` | Array | No | Up to 3 colors for the effect |
| `transitionDuration` | Number | No | Smooth transition time |
| `transitionType` | String | No | Transition easing |

### Examples
```json
// Basic rainbow effect
{
  "effect": {
    "type": "rainbow",
    "speed": 1.5
  }
}

// Fire effect with custom colors
{
  "effect": {
    "type": "fire",
    "colors": ["red", "orange", "yellow"],
    "intensity": 1.2,
    "speed": 0.8
  }
}

// Smooth transition to breathing effect
{
  "effect": {
    "type": "breathing",
    "speed": 0.5,
    "transitionDuration": 2000,
    "transitionType": "ease_in_out"
  }
}
```

---

## Pixels Command

Controls individual LED pixels.

### Syntax
```json
{
  "pixels": {
    "set": [<pixel_objects>],
    "range": <range_object>
  }
}
```

### Pixel Object
```json
{
  "index": <led_index>,
  "color": <color_specification>
}
```

### Range Object
```json
{
  "start": <start_index>,
  "end": <end_index>,
  "color": <color_specification>
}
```

### Examples
```json
// Set individual pixels
{
  "pixels": {
    "set": [
      {"index": 0, "color": "red"},
      {"index": 5, "color": "#00FF00"},
      {"index": 10, "color": {"r": 0, "g": 0, "b": 255}}
    ]
  }
}

// Set range of pixels
{
  "pixels": {
    "range": {
      "start": 0,
      "end": 9,
      "color": "purple"
    }
  }
}
```

---

## Animation Command

Controls the animation system.

### Syntax
```json
{
  "animation": {
    "start": <boolean>,
    "stop": <boolean>,
    "pause": <boolean>
  }
}
```

### Examples
```json
// Start animation
{
  "animation": {
    "start": true
  }
}

// Stop animation
{
  "animation": {
    "stop": true
  }
}
```

---

## Color Specifications

The API supports multiple color formats:

### RGB Object
```json
{
  "r": 255,
  "g": 128,
  "b": 64,
  "a": 255
}
```
- `r`, `g`, `b`: Red, Green, Blue values (0-255)
- `a`: Alpha/brightness (0-255, optional, default: 255)

### HSV Object
```json
{
  "h": 180.0,
  "s": 1.0,
  "v": 0.8
}
```
- `h`: Hue in degrees (0-360)
- `s`: Saturation (0.0-1.0)
- `v`: Value/brightness (0.0-1.0)

### Named Colors
Supported named colors (case-insensitive):
- `"red"`, `"green"`, `"blue"`
- `"white"`, `"black"`
- `"yellow"`, `"cyan"`, `"magenta"`
- `"orange"`, `"purple"`, `"pink"`

### Hex Strings
```json
"#FF8040"  // 6-digit hex
"FF8040"   // Without hash prefix
```

### Integer RGB
```json
16744512  // 0xFF8040 as integer
```

### Named Color Object
```json
{
  "name": "red"
}
```

---

## Effect Types

### Available Effects

| Effect Name | Description | Recommended Colors |
|-------------|-------------|-------------------|
| `"none"` | No effect (static display) | Any |
| `"rainbow"` | Cycling rainbow pattern | Auto-generated |
| `"breathing"` | Fade in/out effect | 1 primary color |
| `"wave"` | Wave propagation effect | 1-2 colors |
| `"sparkle"` | Random sparkle effect | 1-3 colors |
| `"chase"` | Moving dot/segment | 1-2 colors |
| `"fire"` | Fire simulation | Red, orange, yellow |
| `"twinkle"` | Random twinkling stars | 1-2 colors |
| `"meteor"` | Meteor trail effect | 1-2 colors |

### Effect Parameters

#### Speed
- Range: 0.1 - 10.0
- Default: 1.0
- Lower values = slower animation
- Higher values = faster animation

#### Intensity
- Range: 0.0 - 2.0
- Default: 1.0
- Controls effect brightness/prominence
- Values > 1.0 may oversaturate

#### Colors Array
- Maximum 3 colors per effect
- Some effects use only the first color
- Order matters for multi-color effects

---

## Transition Types

### Available Transitions

| Transition Name | Description | Use Case |
|----------------|-------------|----------|
| `"linear"` | Constant rate | Simple, predictable |
| `"ease_in"` | Slow start | Gentle beginnings |
| `"ease_out"` | Slow finish | Smooth endings |
| `"ease_in_out"` | Slow start/finish | Natural motion |
| `"ease_in_quad"` | Quadratic ease in | Subtle acceleration |
| `"ease_out_quad"` | Quadratic ease out | Subtle deceleration |
| `"ease_in_out_quad"` | Quadratic both | Smooth quadratic |
| `"ease_in_cubic"` | Cubic ease in | Strong acceleration |
| `"ease_out_cubic"` | Cubic ease out | Strong deceleration |
| `"ease_in_out_cubic"` | Cubic both | Dramatic motion |
| `"bounce_out"` | Bouncing effect | Playful endings |
| `"elastic_out"` | Elastic overshoot | Spring-like motion |

### Transition Duration
- Specified in milliseconds
- Range: 0 - 4294967295 (uint32_t max)
- Typical values: 500-5000ms
- 0 = instant change

---

## Control Flow

### Sequential Execution (`then`)

Commands can be chained using the `then` property:

#### Single Command
```json
{
  "fill": {"color": "red"},
  "then": {
    "fill": {"color": "blue"}
  }
}
```

#### Command Array
```json
{
  "fill": {"color": "red"},
  "then": [
    {"fill": {"color": "green"}},
    {"fill": {"color": "blue"}},
    {"effect": {"type": "rainbow"}}
  ]
}
```

### Looping (`loop`)

Add `"loop": true` to repeat the entire sequence:

```json
{
  "fill": {
    "color": "red",
    "transitionDuration": 1000
  },
  "then": [
    {
      "fill": {
        "color": "blue",
        "transitionDuration": 1000
      }
    },
    {
      "fill": {
        "color": "green",
        "transitionDuration": 1000
      }
    }
  ],
  "loop": true
}
```

### Nesting and Complexity

- Maximum nesting depth: 10 levels
- Commands within `then` arrays can have their own `then` sequences
- Loops restart from the beginning after completing all `then` commands
- Memory is automatically managed and cleaned up

---

## Examples

### Basic Examples

#### Simple Color Change
```json
{
  "fill": {
    "color": "red"
  }
}
```

#### Smooth Transition
```json
{
  "fill": {
    "color": "blue",
    "transitionDuration": 2000,
    "transitionType": "ease_in_out"
  }
}
```

### Intermediate Examples

#### Color Sequence
```json
{
  "fill": {"color": "red"},
  "then": [
    {
      "fill": {
        "color": "green",
        "transitionDuration": 1000
      }
    },
    {
      "fill": {
        "color": "blue",
        "transitionDuration": 1000
      }
    }
  ]
}
```

#### Effect with Custom Colors
```json
{
  "effect": {
    "type": "fire",
    "colors": [
      {"r": 255, "g": 0, "b": 0},
      {"r": 255, "g": 64, "b": 0},
      {"r": 255, "g": 128, "b": 0}
    ],
    "speed": 1.2,
    "intensity": 1.5
  }
}
```

### Advanced Examples

#### Complex Gradient Sequence
```json
{
  "gradient": {
    "stops": [
      {"color": "red", "position": 0.0},
      {"color": "yellow", "position": 0.33},
      {"color": "green", "position": 0.66},
      {"color": "blue", "position": 1.0}
    ],
    "smooth": true,
    "duration": 3000
  },
  "then": [
    {
      "gradient": {
        "reverse": true,
        "smooth": true,
        "duration": 2000
      }
    },
    {
      "effect": {
        "type": "sparkle",
        "colors": ["white"],
        "speed": 2.0
      }
    }
  ]
}
```

#### Looping Animation Sequence
```json
{
  "fill": {
    "color": "black",
    "transitionDuration": 500
  },
  "then": [
    {
      "effect": {
        "type": "breathing",
        "colors": ["red"],
        "speed": 0.8,
        "transitionDuration": 1000
      }
    },
    {
      "pixels": {
        "set": [
          {"index": 0, "color": "white"},
          {"index": 29, "color": "white"}
        ]
      }
    },
    {
      "gradient": {
        "start": "purple",
        "end": "cyan",
        "smooth": true,
        "duration": 2000
      }
    },
    {
      "effect": {
        "type": "rainbow",
        "speed": 1.5,
        "transitionDuration": 1500
      }
    }
  ],
  "loop": true
}
```

#### Mixed Commands with Timing
```json
{
  "fill": {"color": "black"},
  "pixels": {
    "range": {
      "start": 0,
      "end": 9,
      "color": "red"
    }
  },
  "then": [
    {
      "gradient": {
        "start": "red",
        "end": "blue",
        "smooth": true,
        "duration": 2000,
        "easing": "ease_out_cubic"
      }
    },
    {
      "effect": {
        "type": "wave",
        "colors": ["white", "blue"],
        "speed": 1.2,
        "intensity": 0.8
      },
      "then": {
        "fill": {
          "color": "green",
          "transitionDuration": 1000
        }
      }
    }
  ]
}
```

---

## Error Handling

### Common Error Conditions

1. **Memory Allocation Failures**
   - Large command sequences may fail to allocate
   - System will log error and skip problematic commands
   - Previous state is maintained

2. **Invalid Color Specifications**
   - Invalid colors return `WColor::INVALID`
   - Command is skipped with no visual change
   - Error logged to Serial

3. **Malformed JSON**
   - Parser rejects invalid JSON structures
   - No commands execute
   - System remains in previous state

4. **Out of Range Values**
   - Values are constrained to valid ranges
   - Speed: 0.1-10.0, Intensity: 0.0-2.0
   - Index values checked against strip length

### Error Messages

The system outputs diagnostic messages via Serial:

```
ERROR: Maximum nesting depth exceeded
ERROR: Failed to allocate loop document
ERROR: Failed to allocate memory for then array
WARNING: Empty then array
WARNING: Too many JSON keys, truncating
WARNING: Array too large, truncating
```

### Recovery Behavior

- Failed commands are skipped
- Sequence continues with next valid command
- Memory is cleaned up automatically
- System maintains stable state

---

## Best Practices

### Performance Optimization

1. **Minimize JSON Size**
   - Use short color names when possible
   - Avoid unnecessary nesting
   - Remove optional parameters with default values

2. **Memory Management**
   - Keep command sequences reasonable in size
   - Use loops instead of very long `then` arrays
   - Clean up sequences that are no longer needed

3. **Transition Timing**
   - Use appropriate transition durations (500-5000ms)
   - Avoid very short transitions (< 100ms)
   - Consider LED strip response time

### Visual Design

1. **Color Choices**
   - Test colors on actual hardware
   - Consider LED strip color temperature
   - Account for brightness differences

2. **Effect Parameters**
   - Start with default speed (1.0) and adjust
   - Use intensity values 0.8-1.2 for most effects
   - Test different transition types for desired feel

3. **Sequencing**
   - Plan smooth transitions between commands
   - Use complementary colors for better flow
   - Consider user interaction timing

### Code Organization

1. **JSON Structure**
   - Use consistent indentation
   - Group related commands logically
   - Comment complex sequences (in your application code)

2. **Parameter Validation**
   - Validate JSON before sending to the API
   - Provide fallback values for critical parameters
   - Test edge cases and error conditions

---

## Troubleshooting

### Common Issues

#### Colors Not Displaying Correctly
- **Check Color Format**: Ensure RGB values are 0-255
- **Verify Hex Format**: Use proper 6-digit hex codes
- **Test Named Colors**: Use supported color names
- **Check Alpha Values**: Alpha affects brightness

#### Transitions Not Working
- **Verify Duration**: Ensure `transitionDuration` is specified
- **Check Syntax**: Confirm proper JSON structure
- **Test Simple Cases**: Start with basic transitions
- **Monitor Serial Output**: Check for error messages

#### Sequences Not Executing
- **Validate JSON**: Ensure proper JSON syntax
- **Check Memory**: Monitor available heap memory
- **Simplify Commands**: Break complex sequences into parts
- **Review Nesting**: Stay within 10-level depth limit

#### Effects Not Responding
- **Verify Effect Names**: Use supported effect types
- **Check Parameters**: Ensure values are in valid ranges
- **Test Color Arrays**: Provide appropriate colors for effects
- **Animation State**: Ensure animation system is started

### Debugging Steps

1. **Start Simple**
   ```json
   {"fill": {"color": "red"}}
   ```

2. **Add Transitions**
   ```json
   {
     "fill": {
       "color": "red",
       "transitionDuration": 1000
     }
   }
   ```

3. **Test Sequences**
   ```json
   {
     "fill": {"color": "red"},
     "then": {"fill": {"color": "blue"}}
   }
   ```

4. **Monitor Serial Output**
   - Enable Serial monitoring at 115200 baud
   - Watch for error messages and JSON echo
   - Check memory allocation messages

5. **Memory Monitoring**
   ```cpp
   Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
   ```

### Performance Monitoring

- Monitor frame rates during complex effects
- Check memory usage during long sequences
- Test system stability under load
- Verify cleanup after sequence completion

---

## Appendix

### Complete Color Reference

#### Named Colors (RGB Values)
- `red`: (255, 0, 0)
- `green`: (0, 255, 0)
- `blue`: (0, 0, 255)
- `white`: (255, 255, 255)
- `black`: (0, 0, 0)
- `yellow`: (255, 255, 0)
- `cyan`: (0, 255, 255)
- `magenta`: (255, 0, 255)
- `orange`: (255, 165, 0)
- `purple`: (128, 0, 128)
- `pink`: (255, 192, 203)

### Memory Considerations

#### Allocation Limits
- Single command: ~8KB maximum
- Command arrays: ~16KB maximum
- Total nesting depth: 10 levels
- Array elements: 100 maximum

#### ESP32 Memory Management
- Heap fragmentation considerations
- Dynamic allocation overhead
- Cleanup timing and strategies

### API Version Information

This documentation covers the LED Strip JSON API as implemented in the provided C++ code. The API includes:
- ArduinoJSON library integration
- ESP32 NeoPixel support
- Dynamic memory management
- Comprehensive error handling
- Advanced sequencing capabilities

---

*Last Updated: June 2025*
*Compatible with: ESP32, ArduinoJSON 6.x, Adafruit NeoPixel Library*