#ifndef FOGTOOLMODE_H
#define FOGTOOLMODE_H

enum class FogToolMode {
    UnifiedFog,       // Single unified fog tool with modifier-based behavior
    DrawPen,          // Drawing mode with pen tool (preserved for drawing system)
    DrawEraser        // Drawing mode with eraser tool (preserved for drawing system)
};

#endif // FOGTOOLMODE_H